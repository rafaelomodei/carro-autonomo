#include "services/websocket/ClientRegistry.hpp"

namespace autonomous_car::services::websocket {

void ClientRegistry::addSession(std::size_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    roles_.emplace(session_id, ClientRole::Unknown);
}

void ClientRegistry::removeSession(std::size_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    roles_.erase(session_id);
    if (controller_session_id_ && *controller_session_id_ == session_id) {
        controller_session_id_.reset();
    }
}

bool ClientRegistry::requestRole(std::size_t session_id, ClientRole requested_role) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = roles_.find(session_id);
    if (it == roles_.end()) {
        return false;
    }

    if (requested_role == ClientRole::Telemetry) {
        if (controller_session_id_ && *controller_session_id_ == session_id) {
            controller_session_id_.reset();
        }
        it->second = ClientRole::Telemetry;
        return true;
    }

    if (!controller_session_id_ || *controller_session_id_ == session_id) {
        controller_session_id_ = session_id;
        it->second = ClientRole::Control;
        return true;
    }

    it->second = ClientRole::Telemetry;
    return false;
}

bool ClientRegistry::ensureController(std::size_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = roles_.find(session_id);
    if (it == roles_.end()) {
        return false;
    }

    if (it->second == ClientRole::Control) {
        return true;
    }
    if (it->second == ClientRole::Telemetry) {
        return false;
    }

    if (!controller_session_id_ || *controller_session_id_ == session_id) {
        controller_session_id_ = session_id;
        it->second = ClientRole::Control;
        return true;
    }

    it->second = ClientRole::Telemetry;
    return false;
}

ClientRole ClientRegistry::roleOf(std::size_t session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = roles_.find(session_id);
    if (it == roles_.end()) {
        return ClientRole::Unknown;
    }
    return it->second;
}

std::optional<std::size_t> ClientRegistry::controllerId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_session_id_;
}

std::vector<std::size_t> ClientRegistry::sessionIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::size_t> ids;
    ids.reserve(roles_.size());
    for (const auto &[session_id, _] : roles_) {
        ids.push_back(session_id);
    }
    return ids;
}

} // namespace autonomous_car::services::websocket
