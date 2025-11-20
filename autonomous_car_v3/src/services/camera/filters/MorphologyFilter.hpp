#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Executa uma operação morfológica (close) para preencher buracos e consolidar a pista.
 *
 * Kernels maiores em `LANE_MORPH_KERNEL` e mais iterações (`LANE_MORPH_ITERATIONS`) deixam a linha
 * mais contínua, porém podem juntar áreas indesejadas.
 */
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
