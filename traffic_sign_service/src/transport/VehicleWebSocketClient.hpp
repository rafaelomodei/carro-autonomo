#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "transport/IVehicleTransport.hpp"

namespace traffic_sign_service::transport {

class VehicleWebSocketClient : public IVehicleTransport {
public:
    explicit VehicleWebSocketClient(std::string url);
    ~VehicleWebSocketClient() override;

    void setMessageHandler(MessageHandler handler) override;
    void setOpenHandler(OpenHandler handler) override;
    void start() override;
    void stop() override;
    bool sendText(const std::string &payload) override;

private:
    using Client = websocketpp::client<websocketpp::config::asio_client>;

    void runLoop();
    void waitBeforeReconnect(std::size_t reconnect_attempt);

    std::string url_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    mutable std::mutex state_mutex_;
    MessageHandler message_handler_;
    OpenHandler open_handler_;
    Client *active_client_{nullptr};
    websocketpp::connection_hdl active_connection_;
    bool connected_{false};

    std::condition_variable reconnect_cv_;
    std::mutex reconnect_mutex_;
};

} // namespace traffic_sign_service::transport
