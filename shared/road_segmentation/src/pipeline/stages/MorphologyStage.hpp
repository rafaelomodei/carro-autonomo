#pragma once

#include <opencv2/core.hpp>

namespace road_segmentation_lab::pipeline::stages {

class MorphologyStage {
  public:
    MorphologyStage(bool enabled, cv::Size kernel_size, int iterations);

    cv::Mat apply(const cv::Mat &frame) const;

  private:
    bool enabled_{true};
    cv::Size kernel_size_{5, 5};
    int iterations_{1};
};

} // namespace road_segmentation_lab::pipeline::stages
