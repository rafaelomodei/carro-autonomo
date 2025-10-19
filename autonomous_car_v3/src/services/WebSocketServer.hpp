#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace autonomous_car::controllers {
class CommandDispatcher;
}

namespace autonomous_car::services {

class WebSocketServer {
public:
    using ConfigUpdateHandler = std::function<bool(const std::string &, const std::string &)>;

    WebSocketServer(const std::string &host, int port, controllers::CommandDispatcher &dispatcher,
                    ConfigUpdateHandler config_handler);
    ~WebSocketServer();

    void start();
    void stop();

private:
    void run();

    std::string host_;
    int port_;
    controllers::CommandDispatcher &dispatcher_;
    ConfigUpdateHandler config_handler_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<int> server_fd_;
    std::atomic<int> active_client_fd_;
};

} // namespace autonomous_car::services
