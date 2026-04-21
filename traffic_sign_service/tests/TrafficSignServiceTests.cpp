#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "app/TrafficSignService.hpp"
#include "domain/ITrafficSignClassifier.hpp"
#include "frame_source/IFrameSource.hpp"
#include "transport/IMessagePublisher.hpp"

namespace {

using traffic_sign_service::SignalDetection;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::TrafficSignDetectorState;
using traffic_sign_service::TrafficSignFrameResult;
using traffic_sign_service::TrafficSignInferenceInput;
using traffic_sign_service::app::TrafficSignService;
using traffic_sign_service::config::ServiceConfig;
using traffic_sign_service::frame_source::FramePacket;
using traffic_sign_service::frame_source::IFrameSource;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::tests::expectContains;
using traffic_sign_service::transport::IMessagePublisher;

class FakeFrameSource final : public IFrameSource {
public:
    explicit FakeFrameSource(bool finite) : finite_{finite} {}

    void setFrameHandler(FrameHandler handler) override { frame_handler_ = std::move(handler); }
    void setEndHandler(EndHandler handler) override { end_handler_ = std::move(handler); }
    void start() override { started_ = true; }
    void stop() override { started_ = false; }
    bool isFinite() const noexcept override { return finite_; }
    std::string description() const override { return finite_ ? "Fake finite" : "Fake infinite"; }

    void emitFrame(const cv::Mat &frame, std::uint64_t timestamp_ms) {
        if (frame_handler_) {
            frame_handler_(FramePacket{frame.clone(), timestamp_ms});
        }
    }

    void finish() {
        if (end_handler_) {
            end_handler_();
        }
    }

private:
    bool finite_{false};
    bool started_{false};
    FrameHandler frame_handler_;
    EndHandler end_handler_;
};

class FakeMessagePublisher final : public IMessagePublisher {
public:
    bool sendText(const std::string &payload) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sent_messages_.push_back(payload);
        }
        cv_.notify_all();
        return true;
    }

    bool waitForSentCount(std::size_t expected, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout,
                            [&] { return sent_messages_.size() >= expected; });
    }

    std::vector<std::string> sentMessages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sent_messages_;
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::vector<std::string> sent_messages_;
};

class FakeClassifier final : public traffic_sign_service::ITrafficSignClassifier {
public:
    explicit FakeClassifier(std::vector<TrafficSignFrameResult> results)
        : results_{std::move(results)} {}

    TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                  std::int64_t timestamp_ms) override {
        std::lock_guard<std::mutex> lock(mutex_);
        ++invocations_;

        TrafficSignFrameResult result;
        if (invocations_ <= results_.size()) {
            result = results_[invocations_ - 1];
        }

        if (result.roi.frame_rect.area() <= 0) {
            result.roi = traffic_sign_service::buildTrafficSignRoi(
                input.frame.size(), 0.55, 1.0, 0.08, 0.72, true);
        }
        if (result.timestamp_ms <= 0) {
            result.timestamp_ms = timestamp_ms;
        }
        if (result.debug_frame.empty()) {
            result.debug_frame = input.frame.clone();
        }

        cv_.notify_all();
        return result;
    }

    std::vector<std::string> modelLabels() const override {
        return {"Parada Obrigatoria sign", "Vire a esquerda sign"};
    }

    cv::Size inputSize() const override { return {32, 32}; }

    std::string backendName() const override { return "fake"; }

    bool waitForInvocations(std::size_t expected, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] { return invocations_ >= expected; });
    }

private:
    std::vector<TrafficSignFrameResult> results_;
    std::size_t invocations_{0};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

TrafficSignFrameResult makeRawStopResult(double confidence) {
    TrafficSignFrameResult result;
    result.detector_state = TrafficSignDetectorState::Idle;
    SignalDetection detection;
    detection.signal_id = TrafficSignalId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = traffic_sign_service::displayLabel(TrafficSignalId::Stop);
    detection.confidence = confidence;
    detection.bbox_frame = {100, 50, 40, 40};
    detection.bbox_roi = {10, 10, 40, 40};
    result.raw_detections.push_back(detection);
    return result;
}

void testServiceEmitsSignalAndTelemetryForWebsocketMode() {
    auto *frame_source = new FakeFrameSource(false);
    auto *publisher = new FakeMessagePublisher();
    auto *classifier = new FakeClassifier({makeRawStopResult(0.95), makeRawStopResult(0.96)});

    ServiceConfig config;
    config.frame_source_mode = traffic_sign_service::FrameSourceMode::WebSocket;
    config.frame_preview_enabled = false;
    config.traffic_sign_min_consecutive_frames = 2;
    config.inference_max_fps = 120.0;

    TrafficSignService service(
        config, std::unique_ptr<IFrameSource>(frame_source),
        std::unique_ptr<IMessagePublisher>(publisher),
        std::unique_ptr<traffic_sign_service::ITrafficSignClassifier>(classifier),
        [] { return static_cast<std::uint64_t>(5000); });

    service.start();

    const cv::Mat frame(120, 160, CV_8UC3, cv::Scalar(0, 0, 255));
    frame_source->emitFrame(frame, 1000);
    expect(classifier->waitForInvocations(1, std::chrono::milliseconds(500)),
           "Primeiro frame deve chegar ao classificador.");
    frame_source->emitFrame(frame, 1100);

    expect(classifier->waitForInvocations(2, std::chrono::milliseconds(500)),
           "Dois frames devem chegar ao classificador.");
    expect(publisher->waitForSentCount(3, std::chrono::milliseconds(500)),
           "Modo websocket deve emitir duas telemetrias e um signal:detected.");

    const auto messages = publisher->sentMessages();
    expectContains(messages[0], "\"type\":\"telemetry.traffic_sign_detection\"",
                   "Primeira mensagem deve ser telemetria detalhada.");
    expect(messages[1] == "signal:detected=stop",
           "Segunda mensagem deve emitir o sinal canonico confirmado.");
    expectContains(messages[2], "\"active_detection\":{",
                   "Telemetria confirmada deve conter active_detection.");

    service.stop();
}

void testServiceCompletesFiniteSourceWithoutSendingMessagesOutsideWebsocketMode() {
    auto *frame_source = new FakeFrameSource(true);
    auto *publisher = new FakeMessagePublisher();
    auto *classifier = new FakeClassifier({makeRawStopResult(0.95)});

    ServiceConfig config;
    config.frame_source_mode = traffic_sign_service::FrameSourceMode::Image;
    config.frame_preview_enabled = false;
    config.inference_max_fps = 120.0;

    TrafficSignService service(
        config, std::unique_ptr<IFrameSource>(frame_source),
        std::unique_ptr<IMessagePublisher>(publisher),
        std::unique_ptr<traffic_sign_service::ITrafficSignClassifier>(classifier),
        [] { return static_cast<std::uint64_t>(5000); });

    service.start();
    const cv::Mat frame(120, 160, CV_8UC3, cv::Scalar(0, 255, 0));
    frame_source->emitFrame(frame, 1000);
    frame_source->finish();

    expect(classifier->waitForInvocations(1, std::chrono::milliseconds(500)),
           "Frame de fonte finita deve ser processado.");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (!service.isCompleted() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    expect(service.isCompleted(), "Servico deve encerrar o pipeline ao final da fonte finita.");
    expect(publisher->sentMessages().empty(),
           "Modos locais nao devem publicar mensagens WebSocket.");

    service.stop();
}

TestRegistrar service_ws_test("traffic_sign_service_emits_signal_and_telemetry_in_websocket_mode",
                              testServiceEmitsSignalAndTelemetryForWebsocketMode);
TestRegistrar service_local_completion_test(
    "traffic_sign_service_completes_finite_source_without_websocket_output",
    testServiceCompletesFiniteSourceWithoutSendingMessagesOutsideWebsocketMode);

} // namespace
