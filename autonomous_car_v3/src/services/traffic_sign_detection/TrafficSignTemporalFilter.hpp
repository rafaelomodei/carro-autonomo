#pragma once

#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class TrafficSignTemporalFilter {
public:
    explicit TrafficSignTemporalFilter(TrafficSignConfig config);

    TrafficSignFrameResult update(TrafficSignFrameResult frame_result);
    void reset();

private:
    void clearTrackingState();
    void annotateTrackedDetection(TrafficSignFrameResult &frame_result,
                                  const TrafficSignDetection &tracked_detection) const;
    static bool isSameTrackedSign(const TrafficSignDetection &lhs,
                                  const TrafficSignDetection &rhs);

    TrafficSignConfig config_;
    int missed_frames_{0};
    std::optional<TrafficSignDetection> candidate_detection_;
    std::optional<TrafficSignDetection> active_detection_;
};

} // namespace autonomous_car::services::traffic_sign_detection
