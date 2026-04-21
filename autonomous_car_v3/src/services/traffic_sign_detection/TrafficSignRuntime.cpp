#include "services/traffic_sign_detection/TrafficSignRuntime.hpp"

#include <algorithm>

namespace autonomous_car::services::traffic_sign_detection {

std::int64_t trafficSignResultAgeMs(const TrafficSignFrameResult &result,
                                    std::int64_t reference_timestamp_ms) {
    if (result.timestamp_ms <= 0 || reference_timestamp_ms <= 0) {
        return -1;
    }

    return std::max<std::int64_t>(0, reference_timestamp_ms - result.timestamp_ms);
}

TrafficSignFrameResult buildRenderableTrafficSignResult(const TrafficSignFrameResult &result,
                                                        std::int64_t reference_timestamp_ms,
                                                        std::int64_t max_age_ms) {
    if (max_age_ms < 0) {
        return result;
    }

    const std::int64_t age_ms = trafficSignResultAgeMs(result, reference_timestamp_ms);
    if (age_ms < 0 || age_ms <= max_age_ms) {
        return result;
    }

    TrafficSignFrameResult filtered = result;
    filtered.raw_detections.clear();
    filtered.candidate.reset();
    filtered.active_detection.reset();

    if (filtered.detector_state == TrafficSignDetectorState::Candidate ||
        filtered.detector_state == TrafficSignDetectorState::Confirmed) {
        filtered.detector_state = TrafficSignDetectorState::Idle;
    }

    return filtered;
}

} // namespace autonomous_car::services::traffic_sign_detection
