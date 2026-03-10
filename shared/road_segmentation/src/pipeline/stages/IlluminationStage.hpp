#pragma once

#include <opencv2/core.hpp>

namespace road_segmentation_lab::pipeline::stages {

class IlluminationStage {
  public:
    explicit IlluminationStage(bool clahe_enabled);

    cv::Mat apply(const cv::Mat &frame) const;

  private:
    bool clahe_enabled_{false};
};

} // namespace road_segmentation_lab::pipeline::stages
