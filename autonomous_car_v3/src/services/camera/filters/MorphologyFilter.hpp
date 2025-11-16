#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

class MorphologyFilter : public FrameFilter {
  public:
    MorphologyFilter(int operation, cv::Size kernel_size, int iterations = 1);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    int operation_;
    cv::Size kernel_size_;
    int iterations_;
};

} // namespace autonomous_car::services::camera::filters
