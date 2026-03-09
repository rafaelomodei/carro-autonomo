#pragma once

#include <opencv2/core.hpp>

namespace road_segmentation_lab::pipeline::stages {

class GaussianStage {
  public:
    GaussianStage(bool enabled, cv::Size kernel_size, double sigma);

    cv::Mat apply(const cv::Mat &frame) const;

  private:
    bool enabled_{true};
    cv::Size kernel_size_{5, 5};
    double sigma_{1.2};
};

} // namespace road_segmentation_lab::pipeline::stages
