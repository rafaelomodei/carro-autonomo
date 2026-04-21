#pragma once

#include <functional>
#include <string>

namespace traffic_sign_service::transport {

class IVehicleTransport {
public:
    using MessageHandler = std::function<void(const std::string &)>;
    using OpenHandler = std::function<void()>;

    virtual ~IVehicleTransport() = default;

    virtual void setMessageHandler(MessageHandler handler) = 0;
    virtual void setOpenHandler(OpenHandler handler) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool sendText(const std::string &payload) = 0;
};

} // namespace traffic_sign_service::transport
