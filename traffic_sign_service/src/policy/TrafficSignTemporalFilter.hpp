#pragma once

#include <optional>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service::policy {

struct TrafficSignTemporalFilterConfig {
    double min_confidence{0.60};
    int min_consecutive_frames{2};
    int max_missed_frames{3};
};

class TrafficSignTemporalFilter {
public:
    explicit TrafficSignTemporalFilter(TrafficSignTemporalFilterConfig config = {});

    TrafficSignFrameResult update(TrafficSignFrameResult frame_result);
    void reset();

private:
    void clearTrackingState();
    void annotateTrackedDetection(TrafficSignFrameResult &frame_result,
                                  const SignalDetection &tracked_detection) const;
    static bool isSameTrackedSign(const SignalDetection &lhs, const SignalDetection &rhs);
    std::optional<SignalDetection> selectTrackableDetection(
        const std::vector<SignalDetection> &detections) const;

    TrafficSignTemporalFilterConfig config_;
    int missed_frames_{0};
    std::optional<SignalDetection> candidate_detection_;
    std::optional<SignalDetection> active_detection_;
};

} // namespace traffic_sign_service::policy
