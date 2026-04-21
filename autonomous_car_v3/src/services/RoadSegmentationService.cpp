#include "services/RoadSegmentationService.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <opencv2/highgui.hpp>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationPipeline.hpp"
#include "pipeline/stages/FrameSource.hpp"
#include "services/async/LatestValueMailbox.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"
#include "services/autonomous_control/AutonomousControlTelemetry.hpp"
#include "services/road_segmentation/RoadSegmentationTelemetry.hpp"
#include "services/road_segmentation/VisionRuntimeConfig.hpp"
#include "services/vision/VisionDebugStream.hpp"
#include "services/vision/VisionDebugViewRenderer.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace autonomous_car::services {
namespace {

namespace rsl = road_segmentation_lab;
namespace rs = autonomous_car::services::road_segmentation;
namespace autoctrl = autonomous_car::services::autonomous_control;
namespace async = autonomous_car::services::async;
namespace vision = autonomous_car::services::vision;

struct RuntimeState {
    rs::VisionRuntimeConfig vision_config;
    rsl::config::LabConfig segmentation_config;
    std::unique_ptr<rsl::pipeline::stages::FrameSource> source;
    std::string source_label;
};

struct CoreFrameSnapshot {
    rsl::pipeline::RoadSegmentationResult segmentation_result;
    autoctrl::AutonomousControlSnapshot control_snapshot;
    std::int64_t timestamp_ms{0};
    std::string calibration_status;
};

struct RuntimeMetrics {
    std::atomic<double> core_fps{0.0};
    std::atomic<double> stream_fps{0.0};
    std::atomic<double> stream_encode_ms{0.0};
};

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

class TelemetryTopicQueue {
public:
    void publish(std::string topic, std::string payload) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.find(topic) == pending_.end()) {
            topic_order_.push_back(topic);
        }
        pending_[std::move(topic)] = std::move(payload);
        condition_.notify_one();
    }

    template <typename StopRequested, typename Rep, typename Period>
    std::vector<std::string> waitPopAllFor(const std::chrono::duration<Rep, Period> &timeout,
                                           StopRequested stop_requested) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, timeout,
                            [&] { return !pending_.empty() || stop_requested(); });
        return popAllLocked();
    }

    std::vector<std::string> drain() {
        std::lock_guard<std::mutex> lock(mutex_);
        return popAllLocked();
    }

    void notifyAll() { condition_.notify_all(); }

private:
    std::vector<std::string> popAllLocked() {
        std::vector<std::string> payloads;
        payloads.reserve(topic_order_.size());

        for (const auto &topic : topic_order_) {
            auto it = pending_.find(topic);
            if (it != pending_.end()) {
                payloads.push_back(std::move(it->second));
            }
        }

        pending_.clear();
        topic_order_.clear();
        return payloads;
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::unordered_map<std::string, std::string> pending_;
    std::vector<std::string> topic_order_;
};

void printWarnings(const char *scope, const std::vector<std::string> &warnings) {
    for (const auto &warning : warnings) {
        std::cerr << "[" << scope << "] " << warning << std::endl;
    }
}

std::int64_t currentTimestampMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

bool shouldPublishNow(double max_fps, std::chrono::steady_clock::time_point now,
                      std::chrono::steady_clock::time_point &last_publish) {
    if (max_fps <= 0.0) {
        return true;
    }

    if (last_publish == std::chrono::steady_clock::time_point::min()) {
        last_publish = now;
        return true;
    }

    const auto min_interval = std::chrono::duration<double>(1.0 / max_fps);
    if ((now - last_publish) >= min_interval) {
        last_publish = now;
        return true;
    }

    return false;
}

std::unique_ptr<rsl::pipeline::stages::FrameSource>
createFrameSource(const rs::VisionRuntimeConfig &config, std::vector<std::string> *warnings,
                  std::string *source_label) {
    auto build_camera_source = [&](int index, const std::string &reason)
        -> std::unique_ptr<rsl::pipeline::stages::FrameSource> {
        try {
            auto source = std::make_unique<rsl::pipeline::stages::FrameSource>(
                rsl::pipeline::stages::FrameSource::fromCameraIndex(index));
            if (source_label) {
                *source_label = source->description();
            }
            if (!reason.empty() && warnings) {
                warnings->push_back(reason);
            }
            return source;
        } catch (const std::exception &ex) {
            if (warnings) {
                warnings->push_back("Falha ao abrir camera fallback: " + std::string(ex.what()));
            }
            return nullptr;
        }
    };

    if (config.source_mode == rs::VisionSourceMode::Camera) {
        return build_camera_source(config.camera_index, {});
    }

    try {
        auto source = std::make_unique<rsl::pipeline::stages::FrameSource>(
            rsl::pipeline::stages::FrameSource::fromInputPath(config.source_path));
        if (source_label) {
            *source_label = source->description();
        }
        return source;
    } catch (const std::exception &ex) {
        const std::string warning =
            "Falha ao abrir fonte local (" + config.source_path + "): " + ex.what() +
            ". Tentando fallback para camera.";
        return build_camera_source(config.camera_index, warning);
    }
}

bool loadRuntimeState(const std::string &vision_config_path, RuntimeState &state) {
    state = RuntimeState{};

    std::vector<std::string> vision_warnings;
    rs::loadVisionRuntimeConfigFromFile(vision_config_path, state.vision_config, &vision_warnings);
    printWarnings("RoadSegmentationService/vision", vision_warnings);

    std::vector<std::string> segmentation_warnings;
    rsl::config::loadConfigFromFile(state.vision_config.segmentation_config_path,
                                    state.segmentation_config, &segmentation_warnings);
    printWarnings("RoadSegmentationService/segmentation", segmentation_warnings);

    std::vector<std::string> source_warnings;
    state.source = createFrameSource(state.vision_config, &source_warnings, &state.source_label);
    printWarnings("RoadSegmentationService/source", source_warnings);

    if (!state.source) {
        std::cerr << "[RoadSegmentationService] Nenhuma fonte de video disponivel." << std::endl;
        return false;
    }

    if (state.source_label.empty()) {
        state.source_label = state.source->description();
    }

    return true;
}

vision::VisionRuntimeTelemetry captureVisionRuntimeTelemetry(
    const RuntimeMetrics &metrics, std::string_view source, std::int64_t timestamp_ms,
    std::uint64_t stream_dropped_frames) {
    vision::VisionRuntimeTelemetry telemetry;
    telemetry.timestamp_ms = timestamp_ms;
    telemetry.source = std::string(source);
    telemetry.core_fps = metrics.core_fps.load(std::memory_order_relaxed);
    telemetry.stream_fps = metrics.stream_fps.load(std::memory_order_relaxed);
    telemetry.traffic_sign_fps = 0.0;
    telemetry.traffic_sign_inference_ms = 0.0;
    telemetry.stream_encode_ms = metrics.stream_encode_ms.load(std::memory_order_relaxed);
    telemetry.traffic_sign_dropped_frames = 0;
    telemetry.stream_dropped_frames = stream_dropped_frames;
    telemetry.sign_result_age_ms = -1;
    return telemetry;
}

} // namespace

RoadSegmentationService::RoadSegmentationService(
    std::string vision_config_path, autoctrl::AutonomousControlService *control_service,
    TelemetryPublisher telemetry_publisher, VisionFramePublisher vision_frame_publisher,
    VisionSubscriptionProvider vision_subscription_provider, ControlSink control_sink,
    std::string window_name)
    : vision_config_path_(std::move(vision_config_path)),
      control_service_(control_service),
      telemetry_publisher_(std::move(telemetry_publisher)),
      vision_frame_publisher_(std::move(vision_frame_publisher)),
      vision_subscription_provider_(std::move(vision_subscription_provider)),
      control_sink_(std::move(control_sink)),
      window_name_(std::move(window_name)) {}

RoadSegmentationService::~RoadSegmentationService() { stop(); }

void RoadSegmentationService::start() {
    if (running_.load()) {
        return;
    }

    stop_requested_.store(false);
    worker_ = std::thread(&RoadSegmentationService::run, this);
}

void RoadSegmentationService::stop() {
    stop_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool RoadSegmentationService::isRunning() const noexcept { return running_.load(); }

void RoadSegmentationService::run() {
    running_.store(true);

    try {
        RuntimeState state;
        if (!loadRuntimeState(vision_config_path_, state)) {
            running_.store(false);
            return;
        }

        async::LatestValueMailbox<std::shared_ptr<CoreFrameSnapshot>> stream_mailbox;
        TelemetryTopicQueue telemetry_queue;
        RuntimeMetrics runtime_metrics;

        std::atomic<bool> toggle_pause_requested{false};
        std::atomic<bool> step_requested{false};

        const bool stream_worker_enabled =
            state.vision_config.debug_window_enabled || static_cast<bool>(vision_frame_publisher_);

        auto notifyWorkers = [&] {
            stream_mailbox.notifyAll();
            telemetry_queue.notifyAll();
        };

        auto requestStop = [&] {
            stop_requested_.store(true);
            notifyWorkers();
        };

        std::thread telemetry_thread;
        if (telemetry_publisher_) {
            telemetry_thread = std::thread(
                [&, source_label = state.source_label,
                 telemetry_max_fps = state.vision_config.telemetry_max_fps] {
                    try {
                        auto last_runtime_publish =
                            std::chrono::steady_clock::time_point::min();

                        while (!stop_requested_.load()) {
                            for (auto &payload : telemetry_queue.waitPopAllFor(
                                     std::chrono::milliseconds(50),
                                     [&] { return stop_requested_.load(); })) {
                                telemetry_publisher_(payload);
                            }

                            const auto now = std::chrono::steady_clock::now();
                            if (shouldPublishNow(telemetry_max_fps, now, last_runtime_publish)) {
                                const auto runtime_timestamp_ms = currentTimestampMs();
                                const auto runtime_telemetry = captureVisionRuntimeTelemetry(
                                    runtime_metrics, source_label, runtime_timestamp_ms,
                                    stream_mailbox.droppedCount());
                                telemetry_publisher_(
                                    vision::buildVisionRuntimeTelemetryJson(runtime_telemetry));
                            }
                        }

                        for (auto &payload : telemetry_queue.drain()) {
                            telemetry_publisher_(payload);
                        }
                    } catch (const std::exception &ex) {
                        std::cerr << "[RoadSegmentationService/telemetry] Excecao: "
                                  << ex.what() << std::endl;
                        requestStop();
                    }
                });
        }

        std::thread stream_thread;
        if (stream_worker_enabled) {
            stream_thread = std::thread(
                [&, source_label = state.source_label,
                 segmentation_config = state.segmentation_config,
                 debug_window_enabled = state.vision_config.debug_window_enabled,
                 stream_max_fps = state.vision_config.stream_max_fps,
                 stream_jpeg_quality = state.vision_config.stream_jpeg_quality] {
                    try {
                        vision::VisionDebugViewRenderer vision_renderer;
                        vision::VisionDebugViewSet last_requested_views;
                        auto last_stream_publish =
                            std::chrono::steady_clock::time_point::min();
                        std::shared_ptr<CoreFrameSnapshot> latest_snapshot;
                        const bool local_window_enabled = debug_window_enabled;
                        bool window_created = false;
                        cv::Mat current_dashboard;
                        RateTracker rate_tracker;

                        auto requestedViews = [&] {
                            return vision_subscription_provider_ ? vision_subscription_provider_()
                                                                 : vision::VisionDebugViewSet{};
                        };

                        auto showLocalWindowAndHandleKeys = [&] {
                            if (!local_window_enabled) {
                                return;
                            }

                            if (!window_created) {
                                cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
                                window_created = true;
                            }
                            if (!current_dashboard.empty()) {
                                cv::imshow(window_name_, current_dashboard);
                            }

                            const int key = cv::waitKey(1);
                            switch (key) {
                            case 27:
                            case 'q':
                            case 'Q':
                                requestStop();
                                break;
                            case 'p':
                            case 'P':
                                toggle_pause_requested.store(true);
                                break;
                            case 'n':
                            case 'N':
                                step_requested.store(true);
                                break;
                            default:
                                break;
                            }
                        };

                        while (!stop_requested_.load()) {
                            auto snapshot = stream_mailbox.waitPopFor(
                                std::chrono::milliseconds(local_window_enabled ? 30 : 100),
                                [&] { return stop_requested_.load(); });
                            const bool received_snapshot = snapshot.has_value();
                            if (received_snapshot) {
                                latest_snapshot = std::move(*snapshot);
                            }

                            const auto requested_views = requestedViews();
                            const bool vision_requests_changed =
                                requested_views != last_requested_views;
                            last_requested_views = requested_views;

                            if (!latest_snapshot) {
                                showLocalWindowAndHandleKeys();
                                continue;
                            }

                            const auto runtime_telemetry = captureVisionRuntimeTelemetry(
                                runtime_metrics, source_label, latest_snapshot->timestamp_ms,
                                stream_mailbox.droppedCount());

                            bool should_publish_stream = false;
                            if (vision_frame_publisher_ && !requested_views.empty()) {
                                const auto now = std::chrono::steady_clock::now();
                                if (vision_requests_changed) {
                                    should_publish_stream = true;
                                    last_stream_publish = now;
                                } else if (received_snapshot &&
                                           shouldPublishNow(stream_max_fps, now,
                                                            last_stream_publish)) {
                                    should_publish_stream = true;
                                }
                            }

                            const bool should_update_dashboard =
                                debug_window_enabled && received_snapshot;
                            if (!should_publish_stream && !should_update_dashboard) {
                                showLocalWindowAndHandleKeys();
                                continue;
                            }

                            std::unordered_map<vision::VisionDebugViewId, cv::Mat,
                                               vision::VisionDebugViewIdHash>
                                rendered_views;
                            auto renderView = [&](vision::VisionDebugViewId view) -> const cv::Mat & {
                                auto [it, inserted] = rendered_views.try_emplace(view, cv::Mat{});
                                if (inserted || it->second.empty()) {
                                    it->second = vision_renderer.render(
                                        view, latest_snapshot->segmentation_result,
                                        segmentation_config, source_label,
                                        latest_snapshot->calibration_status,
                                        latest_snapshot->control_snapshot, runtime_telemetry);
                                }
                                return it->second;
                            };

                            double encode_ms = 0.0;
                            if (should_publish_stream) {
                                const auto encode_started = std::chrono::steady_clock::now();
                                for (const auto view : vision::allVisionDebugViews()) {
                                    if (requested_views.find(view) == requested_views.end()) {
                                        continue;
                                    }

                                    const cv::Mat &frame = renderView(view);
                                    const auto payload = vision::buildVisionFrameJson(
                                        view, frame, latest_snapshot->timestamp_ms,
                                        stream_jpeg_quality);
                                    if (payload) {
                                        vision_frame_publisher_(view, *payload);
                                    }
                                }
                                encode_ms = std::chrono::duration<double, std::milli>(
                                                std::chrono::steady_clock::now() - encode_started)
                                                .count();
                                runtime_metrics.stream_encode_ms.store(
                                    encode_ms, std::memory_order_relaxed);
                                runtime_metrics.stream_fps.store(
                                    rate_tracker.mark(std::chrono::steady_clock::now()),
                                    std::memory_order_relaxed);
                            }

                            if (should_update_dashboard) {
                                current_dashboard =
                                    renderView(vision::VisionDebugViewId::Dashboard).clone();
                            }

                            if (local_window_enabled) {
                                showLocalWindowAndHandleKeys();
                            } else if (encode_ms <= 0.0 && received_snapshot) {
                                runtime_metrics.stream_encode_ms.store(0.0,
                                                                       std::memory_order_relaxed);
                            }
                        }

                        if (window_created) {
                            destroyWindow();
                        }
                    } catch (const std::exception &ex) {
                        std::cerr << "[RoadSegmentationService/stream] Excecao: " << ex.what()
                                  << std::endl;
                        requestStop();
                    }
                });
        }

        std::thread core_thread([&, source_label = state.source_label] {
            try {
                rsl::pipeline::RoadSegmentationPipeline pipeline(state.segmentation_config);
                cv::Mat current_frame;
                bool paused = state.source->isStaticImage();
                bool step_once = true;
                bool need_reprocess = true;
                auto last_telemetry_enqueue =
                    std::chrono::steady_clock::time_point::min();
                RateTracker rate_tracker;

                while (!stop_requested_.load()) {
                    if (toggle_pause_requested.exchange(false) && !state.source->isStaticImage()) {
                        paused = !paused;
                    }
                    if (step_requested.exchange(false) && !state.source->isStaticImage() && paused) {
                        step_once = true;
                    }

                    const bool should_read_static_image =
                        state.source->isStaticImage() && (need_reprocess || current_frame.empty());
                    const bool should_read_dynamic_frame =
                        !state.source->isStaticImage() && !need_reprocess &&
                        (!paused || step_once || current_frame.empty());
                    bool processed_frame = false;

                    if (should_read_static_image || should_read_dynamic_frame) {
                        if (!state.source->read(current_frame) || current_frame.empty()) {
                            std::cerr << "[RoadSegmentationService] Falha ao ler a fonte de video."
                                      << std::endl;
                            requestStop();
                            break;
                        }
                    }

                    if (need_reprocess || should_read_static_image || should_read_dynamic_frame) {
                        processed_frame = true;
                        rsl::pipeline::RoadSegmentationResult segmentation_result =
                            pipeline.process(current_frame);
                        const auto timestamp_ms = currentTimestampMs();

                        autoctrl::AutonomousControlSnapshot control_snapshot;
                        if (control_service_) {
                            control_snapshot =
                                control_service_->process(segmentation_result, timestamp_ms);
                            if (control_sink_) {
                                control_sink_(control_snapshot);
                            }
                        } else {
                            control_snapshot.timestamp_ms = timestamp_ms;
                        }

                        auto snapshot = std::make_shared<CoreFrameSnapshot>();
                        snapshot->segmentation_result = std::move(segmentation_result);
                        snapshot->control_snapshot = control_snapshot;
                        snapshot->timestamp_ms = timestamp_ms;
                        snapshot->calibration_status = pipeline.calibrationStatus();

                        runtime_metrics.core_fps.store(
                            rate_tracker.mark(std::chrono::steady_clock::now()),
                            std::memory_order_relaxed);

                        if (stream_worker_enabled) {
                            stream_mailbox.offer(snapshot);
                        }

                        const auto now = std::chrono::steady_clock::now();
                        if (telemetry_publisher_ &&
                            shouldPublishNow(state.vision_config.telemetry_max_fps, now,
                                             last_telemetry_enqueue)) {
                            telemetry_queue.publish(
                                "telemetry.road_segmentation",
                                rs::buildRoadSegmentationTelemetryJson(
                                    snapshot->segmentation_result, source_label, timestamp_ms));
                            if (control_service_) {
                                telemetry_queue.publish(
                                    "telemetry.autonomous_control",
                                    autoctrl::buildAutonomousControlTelemetryJson(control_snapshot));
                            }
                        }

                        step_once = false;
                        need_reprocess = false;
                    }

                    if (state.source->isStaticImage()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    } else if (!processed_frame && paused) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
            } catch (const std::exception &ex) {
                std::cerr << "[RoadSegmentationService/core] Excecao: " << ex.what()
                          << std::endl;
                requestStop();
            }
        });

        if (core_thread.joinable()) {
            core_thread.join();
        }

        requestStop();

        if (stream_thread.joinable()) {
            stream_thread.join();
        }
        if (telemetry_thread.joinable()) {
            telemetry_thread.join();
        }

        destroyWindow();
        if (control_service_) {
            control_service_->stopAutonomous(autoctrl::StopReason::ServiceStop);
            if (control_sink_) {
                control_sink_(control_service_->snapshot());
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "[RoadSegmentationService] Excecao: " << ex.what() << std::endl;
        destroyWindow();
        if (control_service_) {
            control_service_->stopAutonomous(autoctrl::StopReason::ServiceStop);
            if (control_sink_) {
                control_sink_(control_service_->snapshot());
            }
        }
    }

    running_.store(false);
}

void RoadSegmentationService::destroyWindow() const {
    if (window_name_.empty()) {
        return;
    }

    try {
        cv::destroyWindow(window_name_);
    } catch (const cv::Exception &) {
        // Ambiente sem janela criada: nada para destruir.
    }
}

} // namespace autonomous_car::services
