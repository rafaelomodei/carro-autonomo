#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Aplica um desfoque gaussiano para suavizar ruídos.
 *
 * Um kernel maior deixa a pista com contornos mais suaves, mas pode apagar detalhes finos.
 * Ajuste com `LANE_GAUSSIAN_KERNEL` e `LANE_GAUSSIAN_SIGMA` para equilibrar nitidez e estabilidade.
 */
class GaussianBlurFilter : public FrameFilter {
  public:
    GaussianBlurFilter(cv::Size kernel_size, double sigma_x);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    cv::Size kernel_size_;
    double sigma_x_;
};

} // namespace autonomous_car::services::camera::filters
