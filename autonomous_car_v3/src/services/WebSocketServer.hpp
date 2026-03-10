#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/DrivingMode.hpp"
#include "services/websocket/ClientRegistry.hpp"

namespace autonomous_car::controllers {
class CommandRouter;
}

namespace autonomous_car::services {

class WebSocketServer {
public:
    using ConfigUpdateHandler = std::function<bool(const std::string &, const std::string &)>;
    using DrivingModeProvider = std::function<autonomous_car::DrivingMode()>;
    using ClientRole = websocket::ClientRole;

    WebSocketServer(const std::string &host, int port, controllers::CommandRouter &command_router,
                    ConfigUpdateHandler config_handler, DrivingModeProvider mode_provider);
    ~WebSocketServer();

    void start();
    void stop();
    void broadcastText(const std::string &payload);

private:
    struct ClientSession {
        std::size_t id{0};
        int fd{-1};
        std::atomic<bool> alive{true};
        mutable std::mutex send_mutex;
    };

    using ClientSessionPtr = std::shared_ptr<ClientSession>;

    void run();
    void handleClient(const ClientSessionPtr &session);
    std::vector<ClientSessionPtr> snapshotSessions() const;
    bool assignRequestedRole(const ClientSessionPtr &session, ClientRole requested_role);
    bool ensureControllerRole(const ClientSessionPtr &session);
    void removeSession(const ClientSessionPtr &session);
    void shutdownSession(const ClientSessionPtr &session, bool send_close_frame);

    std::string host_;
    int port_;
    controllers::CommandRouter &command_router_;
    ConfigUpdateHandler config_handler_;
    DrivingModeProvider driving_mode_provider_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<int> server_fd_;
    mutable std::mutex clients_mutex_;
    std::unordered_map<std::size_t, ClientSessionPtr> sessions_;
    std::vector<std::thread> client_threads_;
    std::atomic<std::size_t> next_session_id_{1};
    websocket::ClientRegistry client_registry_;
};

} // namespace autonomous_car::services
