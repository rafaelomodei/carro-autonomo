#pragma once

#include "services/camera/filters/FrameFilter.hpp"

namespace autonomous_car::services::camera::filters {

/**
 * @brief Mantém apenas a região inferior do frame, preenchendo o restante com
 * zeros para preservar o tamanho original da imagem.
 */
class BottomMaskFilter : public FrameFilter {
  public:
    explicit BottomMaskFilter(double keep_ratio);

    cv::Mat apply(const cv::Mat &frame) const override;

  private:
    double keep_ratio_;
};

} // namespace autonomous_car::services::camera::filters
