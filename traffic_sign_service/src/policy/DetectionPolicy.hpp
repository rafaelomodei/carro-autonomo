#pragma once

#include <cstdint>

#include <optional>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service::policy {

struct DetectionPolicyConfig {
    std::uint64_t cooldown_ms{2000};
};

class DetectionPolicy {
public:
    explicit DetectionPolicy(DetectionPolicyConfig config = {});

    std::optional<TrafficSignalId> evaluate(const TrafficSignFrameResult &frame_result,
                                            std::uint64_t now_ms);
    void reset();

private:
    DetectionPolicyConfig config_;
    std::optional<TrafficSignalId> active_signal_;
    std::optional<std::uint64_t> last_emitted_at_ms_;
};

} // namespace traffic_sign_service::policy
