#pragma once

#include <memory>
#include <vector>

#include <opencv2/core.hpp>

#include "services/camera/LaneDetectionResult.hpp"
#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera {

class LaneDetector {
  public:
    LaneDetector();

    LaneDetectionResult detect(const cv::Mat &frame) const;

  private:
    LaneDetectionResult analyzeMask(const cv::Mat &mask,
                                    const cv::Size &frame_size) const;

    std::vector<std::unique_ptr<filters::FrameFilter>> filters_;
};

} // namespace autonomous_car::services::camera
