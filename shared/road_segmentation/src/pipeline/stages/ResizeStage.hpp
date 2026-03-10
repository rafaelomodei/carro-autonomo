#pragma once

#include <opencv2/core.hpp>

namespace road_segmentation_lab::pipeline::stages {

class ResizeStage {
  public:
    ResizeStage(bool enabled, cv::Size target_size);

    cv::Mat apply(const cv::Mat &frame) const;

  private:
    bool enabled_{true};
    cv::Size target_size_{320, 240};
};

} // namespace road_segmentation_lab::pipeline::stages
