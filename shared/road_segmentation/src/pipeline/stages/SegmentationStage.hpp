#pragma once

#include <opencv2/core.hpp>

#include <string>

#include "config/LabConfig.hpp"

namespace road_segmentation_lab::pipeline::stages {

class SegmentationStage {
  public:
    explicit SegmentationStage(const config::LabConfig &config);

    cv::Mat apply(const cv::Mat &frame) const;
    std::string modeName() const;

  private:
    config::SegmentationMode mode_{config::SegmentationMode::HsvDark};
    cv::Scalar hsv_low_{0, 0, 0};
    cv::Scalar hsv_high_{180, 110, 200};
    int gray_threshold_{90};
    int adaptive_block_size_{31};
    double adaptive_c_{9.0};
};

} // namespace road_segmentation_lab::pipeline::stages
