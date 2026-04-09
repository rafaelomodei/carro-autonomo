#include "app/TrafficSignService.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "domain/TrafficSignal.hpp"
#include "vision/FrameDecoder.hpp"

namespace {

std::uint64_t defaultNowMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

} // namespace

namespace traffic_sign_service::app {

TrafficSignService::TrafficSignService(config::ServiceConfig config,
                                       std::unique_ptr<transport::IVehicleTransport> transport,
                                       std::unique_ptr<ITrafficSignClassifier> classifier,
                                       NowProvider now_provider)
    : config_{std::move(config)},
      transport_{std::move(transport)},
      classifier_{std::move(classifier)},
      detection_policy_({config_.detection_min_confidence, config_.detection_confirmation_frames,
                         config_.detection_cooldown_ms}),
      frame_preview_window_{config_.frame_preview_enabled},
      now_provider_{now_provider ? std::move(now_provider) : NowProvider(defaultNowMs)} {
    transport_->setOpenHandler([this]() { handleTransportOpened(); });
    transport_->setMessageHandler([this](const std::string &payload) { handleTransportMessage(payload); });
}

TrafficSignService::~TrafficSignService() { stop(); }

void TrafficSignService::start() {
    if (running_.exchange(true)) {
        return;
    }

    inference_thread_ = std::thread(&TrafficSignService::inferenceLoop, this);
    transport_->start();
}

void TrafficSignService::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    latest_frame_store_.notifyAll();
    transport_->stop();

    if (inference_thread_.joinable()) {
        inference_thread_.join();
    }

    frame_preview_window_.close();
}

void TrafficSignService::handleTransportOpened() {
    const bool registered = transport_->sendText("client:telemetry");
    const bool subscribed = transport_->sendText("stream:subscribe=raw");

    std::cout << "[service] client:telemetry => " << (registered ? "ok" : "falhou")
              << std::endl;
    std::cout << "[service] stream:subscribe=raw => " << (subscribed ? "ok" : "falhou")
              << std::endl;
}

void TrafficSignService::handleTransportMessage(const std::string &payload) {
    std::string parse_error;
    const auto frame = vision::parseVisionFrameMessage(payload, &parse_error);
    if (!frame) {
        if (!parse_error.empty()) {
            std::cerr << "[service] payload ignorado: " << parse_error << std::endl;
        }
        return;
    }

    std::cout << "[service] frame raw recebido em " << frame->timestamp_ms << " ms"
              << std::endl;
    latest_frame_store_.push(*frame);
}

void TrafficSignService::inferenceLoop() {
    std::uint64_t last_seen_generation = 0;
    const auto min_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(1.0 / std::max(config_.inference_max_fps, 0.1)));
    auto next_allowed_inference_at = std::chrono::steady_clock::now();

    while (running_.load()) {
        auto snapshot = latest_frame_store_.waitForNext(last_seen_generation, running_);
        if (!snapshot) {
            break;
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

        std::string decode_error;
        cv::Mat frame = vision::decodeVisionFrameImage(snapshot->value, &decode_error);
        if (frame.empty()) {
            std::cerr << "Frame raw ignorado: " << decode_error << std::endl;
            next_allowed_inference_at = std::chrono::steady_clock::now() + min_interval;
            continue;
        }

        frame_preview_window_.show(frame);

        try {
            const auto detections = classifier_->detect(frame);
            const auto primary_detection = selectPrimaryDetection(detections);
            const auto emitted_signal = detection_policy_.evaluate(primary_detection, now_provider_());

            if (emitted_signal) {
                const std::string payload =
                    "signal:detected=" + std::string(toString(*emitted_signal));
                const bool sent = transport_->sendText(payload);
                std::cout << "[service] " << payload << " => "
                          << (sent ? "enviado" : "falhou") << std::endl;
            }
        } catch (const std::exception &ex) {
            std::cerr << "Falha na inferencia Edge Impulse: " << ex.what() << std::endl;
        }

        next_allowed_inference_at = std::chrono::steady_clock::now() + min_interval;
    }
}

std::optional<SignalDetection> TrafficSignService::selectPrimaryDetection(
    const std::vector<SignalDetection> &detections) const {
    if (detections.empty()) {
        return std::nullopt;
    }

    const auto best = std::max_element(
        detections.begin(), detections.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.confidence < rhs.confidence; });
    return *best;
}

} // namespace traffic_sign_service::app
