#pragma once

#include <cstdint>

#include <opencv2/core.hpp>

#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class TrafficSignDetector {
public:
    virtual ~TrafficSignDetector() = default;

    virtual TrafficSignFrameResult detect(const cv::Mat &frame,
                                          std::int64_t timestamp_ms) = 0;
};

} // namespace autonomous_car::services::traffic_sign_detection
