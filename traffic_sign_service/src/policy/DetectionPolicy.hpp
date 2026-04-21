#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "domain/SignalDetection.hpp"

namespace traffic_sign_service::policy {

struct DetectionPolicyConfig {
    double min_confidence{0.60};
    std::size_t confirmation_frames{3};
    std::uint64_t cooldown_ms{2000};
};

class DetectionPolicy {
public:
    explicit DetectionPolicy(DetectionPolicyConfig config = {});

    std::optional<TrafficSignalId> evaluate(const std::optional<SignalDetection> &candidate,
                                            std::uint64_t now_ms);
    void reset();

private:
    DetectionPolicyConfig config_;
    std::optional<TrafficSignalId> current_streak_signal_;
    std::size_t current_streak_frames_{0};
    bool emitted_in_current_streak_{false};
    std::optional<std::uint64_t> last_emitted_at_ms_;
};

} // namespace traffic_sign_service::policy
