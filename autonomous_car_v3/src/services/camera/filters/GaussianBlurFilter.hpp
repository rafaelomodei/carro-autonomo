#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

class GaussianBlurFilter : public FrameFilter {
  public:
    GaussianBlurFilter(cv::Size kernel_size, double sigma_x);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    cv::Size kernel_size_;
    double sigma_x_;
};

} // namespace autonomous_car::services::camera::filters
