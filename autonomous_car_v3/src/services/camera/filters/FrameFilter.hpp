#pragma once

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera::filters {

/**
 * @brief Interface para filtros que transformam um frame em outro frame.
 *
 * Cada filtro deve permanecer sem estado para facilitar a composição e o
 * reuso em diferentes pipelines. A implementação concreta decide se retorna
 * uma cópia ou um novo frame processado.
 */
class FrameFilter {
  public:
    virtual ~FrameFilter() = default;

    virtual cv::Mat apply(const cv::Mat &frame) const = 0;
};

} // namespace autonomous_car::services::camera::filters
