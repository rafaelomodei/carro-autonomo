#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace autonomous_car::services::websocket {

enum class ClientRole { Unknown, Control, Telemetry };

class ClientRegistry {
public:
    void addSession(std::size_t session_id);
    void removeSession(std::size_t session_id);

    bool requestRole(std::size_t session_id, ClientRole requested_role);
    bool ensureController(std::size_t session_id);

    ClientRole roleOf(std::size_t session_id) const;
    std::optional<std::size_t> controllerId() const;
    std::vector<std::size_t> sessionIds() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::size_t, ClientRole> roles_;
    std::optional<std::size_t> controller_session_id_;
};

} // namespace autonomous_car::services::websocket
