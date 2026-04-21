#include "policy/DetectionPolicy.hpp"

namespace traffic_sign_service::policy {

DetectionPolicy::DetectionPolicy(DetectionPolicyConfig config) : config_{config} {}

std::optional<TrafficSignalId> DetectionPolicy::evaluate(
    const TrafficSignFrameResult &frame_result, std::uint64_t now_ms) {
    if (!frame_result.active_detection.has_value() ||
        frame_result.active_detection->signal_id == TrafficSignalId::Unknown) {
        reset();
        return std::nullopt;
    }

    const TrafficSignalId current_signal = frame_result.active_detection->signal_id;
    if (active_signal_.has_value() && *active_signal_ == current_signal) {
        return std::nullopt;
    }

    if (last_emitted_at_ms_ && now_ms - *last_emitted_at_ms_ < config_.cooldown_ms) {
        active_signal_ = current_signal;
        return std::nullopt;
    }

    active_signal_ = current_signal;
    last_emitted_at_ms_ = now_ms;
    return active_signal_;
}

void DetectionPolicy::reset() {
    active_signal_.reset();
}

} // namespace traffic_sign_service::policy
