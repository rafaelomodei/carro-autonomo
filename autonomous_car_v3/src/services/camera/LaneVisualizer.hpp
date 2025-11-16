#pragma once

#include <opencv2/core.hpp>

#include "services/camera/LaneDetectionResult.hpp"

namespace autonomous_car::services::camera {

class LaneVisualizer {
  public:
    cv::Mat buildDebugView(const cv::Mat &frame, const LaneDetectionResult &result) const;

  private:
    void drawLaneOverlay(cv::Mat &frame, const LaneDetectionResult &result) const;
};

} // namespace autonomous_car::services::camera
