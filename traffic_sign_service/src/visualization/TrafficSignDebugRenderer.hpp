#pragma once

#include <opencv2/core.hpp>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service::visualization {

class TrafficSignDebugRenderer {
public:
    cv::Mat render(const TrafficSignFrameResult &result,
                   const TrafficSignRuntimeMetrics &runtime_metrics) const;
};

} // namespace traffic_sign_service::visualization
