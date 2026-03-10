#pragma once

#include <cstddef>
#include <mutex>
#include <unordered_map>

#include "services/vision/VisionDebugStream.hpp"

namespace autonomous_car::services::vision {

class VisionSubscriptionRegistry {
public:
    void addSession(std::size_t session_id);
    void removeSession(std::size_t session_id);
    void replaceSubscriptions(std::size_t session_id, VisionDebugViewSet subscriptions);
    [[nodiscard]] bool hasSubscription(std::size_t session_id, VisionDebugViewId view) const;
    [[nodiscard]] VisionDebugViewSet unionOfRequestedViews() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::size_t, VisionDebugViewSet> subscriptions_;
};

} // namespace autonomous_car::services::vision
