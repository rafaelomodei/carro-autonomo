#pragma once

#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class TrafficSignTemporalFilter {
public:
    explicit TrafficSignTemporalFilter(TrafficSignConfig config = {});

    void updateConfig(const TrafficSignConfig &config);
    void reset();
    [[nodiscard]] TrafficSignFrameResult update(const TrafficSignDetectionFrame &frame,
                                                std::int64_t timestamp_ms);

private:
    [[nodiscard]] TrafficSignFrameResult buildSnapshot(const TrafficSignDetectionFrame &frame) const;
    [[nodiscard]] StabilizedTrafficSignDetection toStabilized(
        const TrafficSignDetection &detection, int consecutive_frames,
        std::int64_t timestamp_ms) const;

    TrafficSignConfig config_;
    std::optional<StabilizedTrafficSignDetection> candidate_;
    std::optional<StabilizedTrafficSignDetection> active_detection_;
    int active_missed_frames_{0};
};

} // namespace autonomous_car::services::traffic_sign_detection
