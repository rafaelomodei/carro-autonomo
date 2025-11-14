#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "common/DrivingMode.hpp"

namespace autonomous_car::controllers {
class CommandDispatcher;
}

namespace autonomous_car::services {

class WebSocketServer {
public:
    using ConfigUpdateHandler = std::function<bool(const std::string &, const std::string &)>;
    using DrivingModeProvider = std::function<autonomous_car::DrivingMode()>;

    WebSocketServer(const std::string &host, int port, controllers::CommandDispatcher &dispatcher,
                    ConfigUpdateHandler config_handler, DrivingModeProvider mode_provider);
    ~WebSocketServer();

    void start();
    void stop();

private:
    void run();

    std::string host_;
    int port_;
    controllers::CommandDispatcher &dispatcher_;
    ConfigUpdateHandler config_handler_;
    DrivingModeProvider driving_mode_provider_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<int> server_fd_;
    std::atomic<int> active_client_fd_;
};

} // namespace autonomous_car::services
