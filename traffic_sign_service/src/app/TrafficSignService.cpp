#include "app/TrafficSignService.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "telemetry/TrafficSignTelemetry.hpp"

namespace {

std::uint64_t defaultNowMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

struct RateTracker {
    std::chrono::steady_clock::time_point last_mark =
        std::chrono::steady_clock::time_point::min();
    double smoothed_fps{0.0};

    double mark(std::chrono::steady_clock::time_point now) {
        if (last_mark == std::chrono::steady_clock::time_point::min()) {
            last_mark = now;
            return smoothed_fps;
        }

        const double elapsed_seconds =
            std::chrono::duration<double>(now - last_mark).count();
        last_mark = now;
        if (elapsed_seconds <= 0.0) {
            return smoothed_fps;
        }

        const double instant_fps = 1.0 / elapsed_seconds;
        smoothed_fps =
            smoothed_fps <= 0.0 ? instant_fps : (smoothed_fps * 0.75) + (instant_fps * 0.25);
        return smoothed_fps;
    }
};

} // namespace

namespace traffic_sign_service::app {

TrafficSignService::TrafficSignService(config::ServiceConfig config,
                                       std::unique_ptr<frame_source::IFrameSource> frame_source,
                                       std::unique_ptr<transport::IMessagePublisher> message_publisher,
                                       std::unique_ptr<ITrafficSignClassifier> classifier,
                                       NowProvider now_provider)
    : config_{std::move(config)},
      frame_source_{std::move(frame_source)},
      message_publisher_{std::move(message_publisher)},
      classifier_{std::move(classifier)},
      temporal_filter_({config_.traffic_sign_min_confidence,
                        config_.traffic_sign_min_consecutive_frames,
                        config_.traffic_sign_max_missed_frames}),
      detection_policy_({config_.detection_cooldown_ms}),
      frame_preview_window_{config_.frame_preview_enabled},
      now_provider_{now_provider ? std::move(now_provider) : NowProvider(defaultNowMs)} {}

TrafficSignService::~TrafficSignService() { stop(); }

void TrafficSignService::start() {
    if (running_.exchange(true)) {
        return;
    }

    source_finished_ = false;
    processing_completed_ = false;
    received_frames_ = 0;
    processed_frames_ = 0;
    last_received_generation_ = 0;
    last_processed_generation_ = 0;
    service_fps_ = 0.0;
    inference_ms_ = 0.0;
    detection_policy_.reset();
    temporal_filter_.reset();

    {
        std::lock_guard<std::mutex> lock(latest_result_mutex_);
        latest_result_.reset();
    }
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        latest_dashboard_.release();
        latest_dashboard_generation_ = 0;
        displayed_dashboard_generation_ = 0;
    }

    frame_source_->setFrameHandler(
        [this](frame_source::FramePacket frame_packet) { handleSourceFrame(std::move(frame_packet)); });
    frame_source_->setEndHandler([this]() { handleSourceFinished(); });

    inference_thread_ = std::thread(&TrafficSignService::inferenceLoop, this);
    frame_source_->start();
}

void TrafficSignService::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    latest_frame_store_.notifyAll();
    if (frame_source_) {
        frame_source_->stop();
    }

    if (inference_thread_.joinable()) {
        inference_thread_.join();
    }

    frame_preview_window_.close();
    processing_completed_ = true;
}

bool TrafficSignService::pollPreview(int delay_ms) {
    if (!frame_preview_window_.enabled()) {
        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        return true;
    }

    cv::Mat dashboard;
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        if (latest_dashboard_generation_ > displayed_dashboard_generation_ &&
            !latest_dashboard_.empty()) {
            dashboard = latest_dashboard_.clone();
            displayed_dashboard_generation_ = latest_dashboard_generation_;
        }
    }

    if (!dashboard.empty()) {
        return frame_preview_window_.show(dashboard);
    }

    return frame_preview_window_.poll(delay_ms);
}

bool TrafficSignService::isCompleted() const noexcept { return processing_completed_.load(); }

bool TrafficSignService::previewEnabled() const noexcept {
    return frame_preview_window_.enabled();
}

void TrafficSignService::handleSourceFrame(frame_source::FramePacket frame_packet) {
    if (!running_.load()) {
        return;
    }

    ++received_frames_;
    const std::uint64_t generation = latest_frame_store_.push(std::move(frame_packet));
    last_received_generation_ = generation;
}

void TrafficSignService::handleSourceFinished() {
    source_finished_ = true;
    latest_frame_store_.notifyAll();
}

void TrafficSignService::inferenceLoop() {
    RateTracker rate_tracker;
    std::uint64_t last_seen_generation = 0;
    const auto min_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(1.0 / std::max(config_.inference_max_fps, 0.1)));
    auto next_allowed_inference_at = std::chrono::steady_clock::now();

    while (running_.load()) {
        auto snapshot = latest_frame_store_.waitForNext(
            last_seen_generation, running_,
            [this, last_seen_generation] {
                return source_finished_.load() &&
                       last_received_generation_.load() <= last_seen_generation;
            });
        if (!snapshot) {
            if (source_finished_.load() &&
                last_processed_generation_.load() >= last_received_generation_.load()) {
                break;
            }
            continue;
        }
        last_seen_generation = snapshot->generation;

        const auto now = std::chrono::steady_clock::now();
        if (now < next_allowed_inference_at) {
            std::this_thread::sleep_until(next_allowed_inference_at);
            if (!running_.load()) {
                break;
            }

            const auto latest_snapshot = latest_frame_store_.latest();
            if (latest_snapshot && latest_snapshot->generation > last_seen_generation) {
                snapshot = latest_snapshot;
                last_seen_generation = latest_snapshot->generation;
            }
        }

        const auto inference_started = std::chrono::steady_clock::now();
        TrafficSignInferenceInput input;
        input.frame = snapshot->value.frame;
        input.full_frame_size = snapshot->value.frame.size();
        input.capture_debug_frames = config_.frame_preview_enabled;

        TrafficSignFrameResult frame_result =
            classifier_->detect(input, static_cast<std::int64_t>(snapshot->value.timestamp_ms));
        frame_result = temporal_filter_.update(std::move(frame_result));

        const auto inference_finished = std::chrono::steady_clock::now();
        const double inference_ms =
            std::chrono::duration<double, std::milli>(inference_finished - inference_started)
                .count();
        inference_ms_ = inference_ms;
        service_fps_ = rate_tracker.mark(inference_finished);

        {
            std::lock_guard<std::mutex> lock(latest_result_mutex_);
            latest_result_ = frame_result;
        }

        if (config_.frame_source_mode == FrameSourceMode::WebSocket) {
            const auto signal = detection_policy_.evaluate(frame_result, now_provider_());
            if (signal.has_value()) {
                const std::string payload =
                    "signal:detected=" + std::string(toString(*signal));
                const bool sent = message_publisher_ && message_publisher_->sendText(payload);
                std::cout << "[service] " << payload << " => "
                          << (sent ? "enviado" : "falhou") << std::endl;
            }

            const std::string telemetry_payload =
                telemetry::buildTrafficSignTelemetryJson(frame_result, frame_source_->description());
            if (message_publisher_) {
                message_publisher_->sendText(telemetry_payload);
            }
        }

        if (config_.frame_preview_enabled) {
            updateLatestDashboard(
                debug_renderer_.render(frame_result, buildRuntimeMetrics(frame_result)));
        }

        if (processed_frames_.load() == 0) {
            std::cout << "[service] primeiro frame processado. estado: "
                      << toString(frame_result.detector_state) << std::endl;
        }
        ++processed_frames_;
        last_processed_generation_ = snapshot->generation;
        next_allowed_inference_at = std::chrono::steady_clock::now() + min_interval;
    }

    processing_completed_ = true;
    std::cout << "[service] processamento encerrado. frames recebidos: "
              << received_frames_.load() << " | processados: " << processed_frames_.load()
              << std::endl;
}

void TrafficSignService::updateLatestDashboard(cv::Mat dashboard) {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    latest_dashboard_ = std::move(dashboard);
    ++latest_dashboard_generation_;
}

TrafficSignRuntimeMetrics TrafficSignService::buildRuntimeMetrics(
    const TrafficSignFrameResult &result) const {
    TrafficSignRuntimeMetrics metrics;
    metrics.source_label = frame_source_ ? frame_source_->description() : "";
    metrics.source_mode = std::string(toString(config_.frame_source_mode));
    metrics.backend = classifier_ ? classifier_->backendName() : "";
    metrics.service_fps = service_fps_.load();
    metrics.inference_ms = inference_ms_.load();
    metrics.sign_result_age_ms =
        trafficSignResultAgeMs(result, static_cast<std::int64_t>(now_provider_()));
    metrics.frames_received = received_frames_.load();
    metrics.frames_processed = processed_frames_.load();
    metrics.dropped_frames = metrics.frames_received > metrics.frames_processed
                                 ? metrics.frames_received - metrics.frames_processed
                                 : 0;
    return metrics;
}

} // namespace traffic_sign_service::app
