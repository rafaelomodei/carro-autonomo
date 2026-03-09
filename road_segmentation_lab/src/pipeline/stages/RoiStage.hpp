#pragma once

#include <opencv2/core.hpp>

namespace road_segmentation_lab::pipeline::stages {

class RoiStage {
  public:
    RoiStage(double top_ratio, double bottom_ratio);

    cv::Rect computeRect(const cv::Size &frame_size) const;
    cv::Mat apply(const cv::Mat &frame, cv::Rect *rect = nullptr) const;

  private:
    double top_ratio_{0.5};
    double bottom_ratio_{1.0};
};

} // namespace road_segmentation_lab::pipeline::stages
