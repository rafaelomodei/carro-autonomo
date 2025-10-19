#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace autonomous_car::controllers {
class CommandDispatcher;
}

namespace autonomous_car::services {

class WebSocketServer {
public:
    WebSocketServer(const std::string &host, int port, controllers::CommandDispatcher &dispatcher);
    ~WebSocketServer();

    void start();
    void stop();

private:
    void run();

    std::string host_;
    int port_;
    controllers::CommandDispatcher &dispatcher_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<int> server_fd_;
};

} // namespace autonomous_car::services
