#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "TestRegistry.hpp"
#include "frame_source/WebSocketFrameSource.hpp"
#include "transport/IVehicleTransport.hpp"
#include "vision/Base64.hpp"

namespace {

using traffic_sign_service::frame_source::WebSocketFrameSource;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::transport::IVehicleTransport;
using traffic_sign_service::vision::encodeBase64;

class FakeVehicleTransport final : public IVehicleTransport {
public:
    void setMessageHandler(MessageHandler handler) override { message_handler_ = std::move(handler); }
    void setOpenHandler(OpenHandler handler) override { open_handler_ = std::move(handler); }
    void start() override {}
    void stop() override {}

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
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::vector<std::string> sent_messages_;
};

std::string buildValidVisionFramePayload() {
    cv::Mat image(8, 8, CV_8UC3, cv::Scalar(0, 0, 255));
    std::vector<unsigned char> encoded_bytes;
    cv::imencode(".jpg", image, encoded_bytes);

    return std::string(
               R"({"type":"vision.frame","view":"raw","timestamp_ms":1,"mime":"image/jpeg","width":8,"height":8,"data":")") +
           encodeBase64(encoded_bytes) + R"("})";
}

void testWebSocketFrameSourceSubscribesOnOpen() {
    auto transport = std::make_shared<FakeVehicleTransport>();
    WebSocketFrameSource source(transport, "WebSocket: test");

    source.start();
    transport->emitOpen();

    expect(transport->waitForSentCount(2, std::chrono::milliseconds(250)),
           "Ao abrir o socket a fonte deve se registrar e assinar raw.");
    const auto messages = transport->sentMessages();
    expect(messages[0] == "client:telemetry", "Primeira mensagem deve registrar telemetry.");
    expect(messages[1] == "stream:subscribe=raw",
           "Segunda mensagem deve assinar a view raw.");
}

void testWebSocketFrameSourceDecodesRawFrame() {
    auto transport = std::make_shared<FakeVehicleTransport>();
    WebSocketFrameSource source(transport, "WebSocket: test");

    std::mutex mutex;
    std::condition_variable cv;
    bool received = false;
    source.setFrameHandler([&](const auto &frame_packet) {
        std::lock_guard<std::mutex> lock(mutex);
        received = !frame_packet.frame.empty() && frame_packet.timestamp_ms == 1;
        cv.notify_all();
    });

    source.start();
    transport->emitMessage(buildValidVisionFramePayload());

    std::unique_lock<std::mutex> lock(mutex);
    expect(cv.wait_for(lock, std::chrono::milliseconds(250), [&] { return received; }),
           "Payload raw valido deve virar frame decodificado.");
}

TestRegistrar websocket_frame_source_open_test(
    "websocket_frame_source_subscribes_on_open", testWebSocketFrameSourceSubscribesOnOpen);
TestRegistrar websocket_frame_source_decode_test(
    "websocket_frame_source_decodes_raw_frames", testWebSocketFrameSourceDecodesRawFrame);

} // namespace
