#pragma once

#include <opencv2/core.hpp>

#include "services/camera/LaneDetectionResult.hpp"

namespace autonomous_car::services::camera {

class LaneVisualizer {
  public:
    cv::Mat buildDebugView(const cv::Mat &frame, const LaneDetectionResult &result) const;

  private:
    void drawLaneOverlay(cv::Mat &frame, const LaneDetectionResult &result) const;
    cv::Point computeHorizonPoint(const LaneDetectionResult &result, int image_height) const;
    cv::Mat toBinaryMask(const cv::Mat &mask) const;
    cv::Mat buildColoredMask(const cv::Mat &binary_mask) const;
    cv::Mat applyMaskOverlay(const cv::Mat &frame, const cv::Mat &colored_mask,
                             const cv::Mat &binary_mask) const;
};

} // namespace autonomous_car::services::camera
