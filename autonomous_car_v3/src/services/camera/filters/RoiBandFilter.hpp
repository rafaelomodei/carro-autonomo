#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Mantém apenas uma faixa vertical do frame definida por porcentagens.
 *
 * Permite descartar simultaneamente porções superiores e inferiores do frame, focando
 * somente na banda em que a pista costuma aparecer. Os limites são expressos como
 * frações relativas à altura total (0 = topo, 1 = base).
 */
class RoiBandFilter : public FrameFilter {
  public:
    RoiBandFilter(double start_ratio, double end_ratio);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    double start_ratio_;
    double end_ratio_;
};

} // namespace autonomous_car::services::camera::filters
