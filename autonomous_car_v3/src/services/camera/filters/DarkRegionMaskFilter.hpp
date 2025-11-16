#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Aplica um threshold em HSV para destacar regiões escuras (EVA).
 */
class DarkRegionMaskFilter : public FrameFilter {
  public:
    DarkRegionMaskFilter(cv::Scalar lower, cv::Scalar upper);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    cv::Scalar lower_;
    cv::Scalar upper_;
};

} // namespace autonomous_car::services::camera::filters
