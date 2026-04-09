#include "policy/DetectionPolicy.hpp"

namespace traffic_sign_service::policy {

DetectionPolicy::DetectionPolicy(DetectionPolicyConfig config) : config_{config} {}

std::optional<TrafficSignalId> DetectionPolicy::evaluate(
    const std::optional<SignalDetection> &candidate, std::uint64_t now_ms) {
    if (!candidate || candidate->confidence < config_.min_confidence) {
        reset();
        return std::nullopt;
    }

    if (!current_streak_signal_ || *current_streak_signal_ != candidate->signal_id) {
        current_streak_signal_ = candidate->signal_id;
        current_streak_frames_ = 1;
        emitted_in_current_streak_ = false;
    } else {
        ++current_streak_frames_;
    }

    if (current_streak_frames_ < config_.confirmation_frames || emitted_in_current_streak_) {
        return std::nullopt;
    }

    if (last_emitted_at_ms_ && now_ms - *last_emitted_at_ms_ < config_.cooldown_ms) {
        return std::nullopt;
    }

    emitted_in_current_streak_ = true;
    last_emitted_at_ms_ = now_ms;
    return current_streak_signal_;
}

void DetectionPolicy::reset() {
    current_streak_signal_.reset();
    current_streak_frames_ = 0;
    emitted_in_current_streak_ = false;
}

} // namespace traffic_sign_service::policy
