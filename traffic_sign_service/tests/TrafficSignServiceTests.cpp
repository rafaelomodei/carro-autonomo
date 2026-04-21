#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "TestRegistry.hpp"
#include "app/TrafficSignService.hpp"
#include "domain/ITrafficSignClassifier.hpp"
#include "domain/TrafficSignal.hpp"
#include "transport/IVehicleTransport.hpp"
#include "vision/Base64.hpp"

namespace {

using traffic_sign_service::SignalDetection;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::app::TrafficSignService;
using traffic_sign_service::config::ServiceConfig;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::transport::IVehicleTransport;
using traffic_sign_service::vision::encodeBase64;

class FakeVehicleTransport final : public IVehicleTransport {
public:
    void setMessageHandler(MessageHandler handler) override { message_handler_ = std::move(handler); }
    void setOpenHandler(OpenHandler handler) override { open_handler_ = std::move(handler); }

    void start() override { started_ = true; }

    void stop() override {
        started_ = false;
        cv_.notify_all();
    }

    bool sendText(const std::string &payload) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sent_messages_.push_back(payload);
        }
        cv_.notify_all();
        return true;
    }

    void emitOpen() {
        if (open_handler_) {
            open_handler_();
        }
    }

    void emitMessage(const std::string &payload) {
        if (message_handler_) {
            message_handler_(payload);
        }
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
    MessageHandler message_handler_;
    OpenHandler open_handler_;
    bool started_{false};
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::vector<std::string> sent_messages_;
};

class FakeClassifier final : public traffic_sign_service::ITrafficSignClassifier {
public:
    explicit FakeClassifier(std::vector<std::vector<SignalDetection>> detections_per_call)
        : detections_per_call_{std::move(detections_per_call)} {}

    std::vector<SignalDetection> detect(const cv::Mat &) override {
        std::lock_guard<std::mutex> lock(mutex_);
        ++invocations_;
        if (invocations_ <= detections_per_call_.size()) {
            cv_.notify_all();
            return detections_per_call_[invocations_ - 1];
        }
        cv_.notify_all();
        return {};
    }

    bool waitForInvocations(std::size_t expected, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] { return invocations_ >= expected; });
    }

private:
    std::vector<std::vector<SignalDetection>> detections_per_call_;
    std::size_t invocations_{0};
    std::mutex mutex_;
    std::condition_variable cv_;
};

std::string buildValidVisionFramePayload() {
    cv::Mat image(8, 8, CV_8UC3, cv::Scalar(0, 0, 255));
    std::vector<unsigned char> encoded_bytes;
    cv::imencode(".jpg", image, encoded_bytes);

    return std::string(R"({"type":"vision.frame","view":"raw","timestamp_ms":1,"mime":"image/jpeg","width":8,"height":8,"data":")") +
           encodeBase64(encoded_bytes) + R"("})";
}

SignalDetection makeStopDetection() {
    SignalDetection detection;
    detection.signal_id = TrafficSignalId::Stop;
    detection.confidence = 0.91;
    return detection;
}

void testServiceSubscribesOnOpenAndReconnect() {
    auto *transport = new FakeVehicleTransport();
    auto *classifier = new FakeClassifier({});

    ServiceConfig config;
    config.inference_max_fps = 60.0;
    TrafficSignService service(config, std::unique_ptr<IVehicleTransport>(transport),
                               std::unique_ptr<traffic_sign_service::ITrafficSignClassifier>(classifier),
                               [] { return static_cast<std::uint64_t>(1000); });

    service.start();
    transport->emitOpen();
    expect(transport->waitForSentCount(2, std::chrono::milliseconds(250)),
           "Ao abrir o socket o servico deve se registrar como telemetry e assinar raw.");

    transport->emitOpen();
    expect(transport->waitForSentCount(4, std::chrono::milliseconds(250)),
           "Ao reconectar o servico deve reenviar os comandos de registro e subscribe.");

    const auto sent_messages = transport->sentMessages();
    expect(sent_messages[0] == "client:telemetry", "Primeira mensagem deve registrar telemetry.");
    expect(sent_messages[1] == "stream:subscribe=raw",
           "Segunda mensagem deve assinar apenas a view raw.");
    expect(sent_messages[2] == "client:telemetry",
           "Reconexao deve repetir o registro telemetry.");
    expect(sent_messages[3] == "stream:subscribe=raw",
           "Reconexao deve repetir a assinatura raw.");

    service.stop();
}

void testServiceEmitsSingleEventForStableDetection() {
    auto *transport = new FakeVehicleTransport();
    auto *classifier = new FakeClassifier({
        {makeStopDetection()},
        {makeStopDetection()},
        {makeStopDetection()},
        {makeStopDetection()},
        {},
        {makeStopDetection()},
        {makeStopDetection()},
        {makeStopDetection()},
    });

    std::uint64_t now_ms = 1000;
    ServiceConfig config;
    config.inference_max_fps = 120.0;
    TrafficSignService service(config, std::unique_ptr<IVehicleTransport>(transport),
                               std::unique_ptr<traffic_sign_service::ITrafficSignClassifier>(classifier),
                               [&now_ms] { return now_ms; });

    service.start();
    transport->emitOpen();
    expect(transport->waitForSentCount(2, std::chrono::milliseconds(250)),
           "Servico deve concluir handshake do consumidor telemetry.");

    const auto frame_payload = buildValidVisionFramePayload();

    now_ms = 1000;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(1, std::chrono::milliseconds(500)),
           "Primeiro frame deve chegar ao classificador.");

    now_ms = 1100;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(2, std::chrono::milliseconds(500)),
           "Segundo frame deve chegar ao classificador.");

    now_ms = 1200;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(3, std::chrono::milliseconds(500)),
           "Terceiro frame deve chegar ao classificador.");
    expect(transport->waitForSentCount(3, std::chrono::milliseconds(500)),
           "Deteccao confirmada deve gerar um unico signal:detected.");

    now_ms = 4000;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(4, std::chrono::milliseconds(500)),
           "Frames repetidos devem continuar sendo processados.");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    expect(transport->sentMessages().size() == 3,
           "Mesma sequencia de frames nao deve gerar evento duplicado.");

    now_ms = 4100;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(5, std::chrono::milliseconds(500)),
           "Frame vazio deve quebrar a sequencia atual.");

    now_ms = 5000;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(6, std::chrono::milliseconds(500)),
           "Nova aparicao 1 deve ser processada.");

    now_ms = 5100;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(7, std::chrono::milliseconds(500)),
           "Nova aparicao 2 deve ser processada.");

    now_ms = 5200;
    transport->emitMessage(frame_payload);
    expect(classifier->waitForInvocations(8, std::chrono::milliseconds(500)),
           "Nova aparicao 3 deve ser processada.");
    expect(transport->waitForSentCount(4, std::chrono::milliseconds(500)),
           "Reaparicao estavel apos reset deve emitir novo evento.");

    const auto sent_messages = transport->sentMessages();
    expect(sent_messages[2] == "signal:detected=stop",
           "Primeiro evento emitido deve apontar para stop.");
    expect(sent_messages[3] == "signal:detected=stop",
           "Reaparicao confirmada deve reenviar apenas o mesmo sinal canonical.");

    service.stop();
}

TestRegistrar service_open_test("traffic_sign_service_resubscribes_after_reconnect",
                                testServiceSubscribesOnOpenAndReconnect);
TestRegistrar service_emit_test("traffic_sign_service_emits_single_signal_events",
                                testServiceEmitsSingleEventForStableDetection);

} // namespace
