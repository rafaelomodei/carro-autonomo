#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Recorta o trecho inferior do frame para focar na área onde a pista aparece.
 *
 * Esse filtro reduz ruído visual vindo do horizonte. O percentual preservado pode ser
 * controlado via `LANE_ROI_KEEP_RATIO`.
 */
class BottomMaskFilter : public FrameFilter {
  public:
    explicit BottomMaskFilter(double keep_ratio);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    double keep_ratio_;
};

} // namespace autonomous_car::services::camera::filters
