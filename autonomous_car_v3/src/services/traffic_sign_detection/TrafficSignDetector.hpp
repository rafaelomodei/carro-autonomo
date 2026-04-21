#pragma once

#include <cstdint>
#include <optional>

#include <opencv2/core.hpp>

#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

struct TrafficSignInferenceInput {
    cv::Mat frame;
    cv::Size full_frame_size;
    std::optional<TrafficSignRoi> roi;
    bool capture_debug_frames{false};
};

class TrafficSignDetector {
public:
    virtual ~TrafficSignDetector() = default;

    virtual TrafficSignFrameResult detect(const cv::Mat &frame,
                                          std::int64_t timestamp_ms) = 0;

    virtual TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                          std::int64_t timestamp_ms) {
        return detect(input.frame, timestamp_ms);
    }
};

} // namespace autonomous_car::services::traffic_sign_detection
