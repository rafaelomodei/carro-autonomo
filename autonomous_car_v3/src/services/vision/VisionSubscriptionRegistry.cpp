#include "services/vision/VisionSubscriptionRegistry.hpp"

namespace autonomous_car::services::vision {

void VisionSubscriptionRegistry::addSession(std::size_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.try_emplace(session_id);
}

void VisionSubscriptionRegistry::removeSession(std::size_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(session_id);
}

void VisionSubscriptionRegistry::replaceSubscriptions(
    std::size_t session_id, VisionDebugViewSet subscriptions) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[session_id] = std::move(subscriptions);
}

bool VisionSubscriptionRegistry::hasSubscription(
    std::size_t session_id, VisionDebugViewId view) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = subscriptions_.find(session_id);
    if (iterator == subscriptions_.end()) {
        return false;
    }

    return iterator->second.find(view) != iterator->second.end();
}

VisionDebugViewSet VisionSubscriptionRegistry::unionOfRequestedViews() const {
    std::lock_guard<std::mutex> lock(mutex_);

    VisionDebugViewSet requested_views;
    for (const auto &[_, session_subscriptions] : subscriptions_) {
        requested_views.insert(session_subscriptions.begin(), session_subscriptions.end());
    }

    return requested_views;
}

} // namespace autonomous_car::services::vision
