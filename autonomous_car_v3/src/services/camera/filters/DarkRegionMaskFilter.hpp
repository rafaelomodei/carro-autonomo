#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Aplica um threshold em HSV para destacar o EVA preto e suprimir o fundo claro.
 *
 * Alterar `LANE_HSV_LOW` e `LANE_HSV_HIGH` ajuda a refinar quais tons são considerados pista.
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
