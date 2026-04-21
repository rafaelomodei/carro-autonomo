#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "frame_source/IFrameSource.hpp"
#include "transport/IVehicleTransport.hpp"

namespace traffic_sign_service::frame_source {

class WebSocketFrameSource final : public IFrameSource {
public:
    WebSocketFrameSource(std::shared_ptr<transport::IVehicleTransport> transport,
                         std::string description);
    ~WebSocketFrameSource() override;

    void setFrameHandler(FrameHandler handler) override;
    void setEndHandler(EndHandler handler) override;
    void start() override;
    void stop() override;
    bool isFinite() const noexcept override { return false; }
    std::string description() const override;

private:
    void handleOpen();
    void handleMessage(const std::string &payload);

    std::shared_ptr<transport::IVehicleTransport> transport_;
    std::string description_;
    FrameHandler frame_handler_;
    EndHandler end_handler_;
    mutable std::mutex handler_mutex_;
};

} // namespace traffic_sign_service::frame_source
