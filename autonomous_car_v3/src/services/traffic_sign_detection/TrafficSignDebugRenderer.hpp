#pragma once

#include <opencv2/core.hpp>

#include "services/traffic_sign_detection/TrafficSignTypes.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class TrafficSignDebugRenderer {
public:
    cv::Mat render(const TrafficSignFrameResult &result,
                   const autonomous_car::services::vision::VisionRuntimeTelemetry
                       &runtime_telemetry) const;
};

} // namespace autonomous_car::services::traffic_sign_detection
