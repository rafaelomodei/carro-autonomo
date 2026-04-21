#include "frame_source/WebSocketFrameSource.hpp"

#include <iostream>

#include "vision/FrameDecoder.hpp"
#include "vision/VisionFrame.hpp"

namespace traffic_sign_service::frame_source {

WebSocketFrameSource::WebSocketFrameSource(
    std::shared_ptr<transport::IVehicleTransport> transport, std::string description)
    : transport_{std::move(transport)}, description_{std::move(description)} {}

WebSocketFrameSource::~WebSocketFrameSource() { stop(); }

void WebSocketFrameSource::setFrameHandler(FrameHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    frame_handler_ = std::move(handler);
}

void WebSocketFrameSource::setEndHandler(EndHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    end_handler_ = std::move(handler);
}

void WebSocketFrameSource::start() {
    if (!transport_) {
        return;
    }

    transport_->setOpenHandler([this]() { handleOpen(); });
    transport_->setMessageHandler([this](const std::string &payload) { handleMessage(payload); });
    transport_->start();
}

void WebSocketFrameSource::stop() {
    if (transport_) {
        transport_->stop();
    }

    EndHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = end_handler_;
    }
    if (handler) {
        handler();
    }
}

std::string WebSocketFrameSource::description() const { return description_; }

void WebSocketFrameSource::handleOpen() {
    if (!transport_) {
        return;
    }

    const bool registered = transport_->sendText("client:telemetry");
    const bool subscribed = transport_->sendText("stream:subscribe=raw");
    std::cout << "[source] client:telemetry => " << (registered ? "ok" : "falhou")
              << std::endl;
    std::cout << "[source] stream:subscribe=raw => " << (subscribed ? "ok" : "falhou")
              << std::endl;
}

void WebSocketFrameSource::handleMessage(const std::string &payload) {
    std::string parse_error;
    const auto frame_message = vision::parseVisionFrameMessage(payload, &parse_error);
    if (!frame_message) {
        if (!parse_error.empty()) {
            std::cerr << "[source] payload ignorado: " << parse_error << std::endl;
        }
        return;
    }

    std::string decode_error;
    cv::Mat frame = vision::decodeVisionFrameImage(*frame_message, &decode_error);
    if (frame.empty()) {
        std::cerr << "[source] frame websocket ignorado: " << decode_error << std::endl;
        return;
    }

    FrameHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = frame_handler_;
    }
    if (handler) {
        handler(FramePacket{frame, frame_message->timestamp_ms});
    }
}

} // namespace traffic_sign_service::frame_source
