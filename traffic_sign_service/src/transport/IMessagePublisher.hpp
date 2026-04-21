#pragma once

#include <memory>
#include <string>

#include "transport/IVehicleTransport.hpp"

namespace traffic_sign_service::transport {

class IMessagePublisher {
public:
    virtual ~IMessagePublisher() = default;
    virtual bool sendText(const std::string &payload) = 0;
};

class NullMessagePublisher final : public IMessagePublisher {
public:
    bool sendText(const std::string &) override { return false; }
};

class TransportMessagePublisher final : public IMessagePublisher {
public:
    explicit TransportMessagePublisher(std::shared_ptr<IVehicleTransport> transport)
        : transport_{std::move(transport)} {}

    bool sendText(const std::string &payload) override {
        return transport_ ? transport_->sendText(payload) : false;
    }

private:
    std::shared_ptr<IVehicleTransport> transport_;
};

} // namespace traffic_sign_service::transport
