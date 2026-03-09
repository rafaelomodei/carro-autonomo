#pragma once

#include <opencv2/core.hpp>

#include <string>

#include "pipeline/RoadSegmentationResult.hpp"

namespace road_segmentation_lab::pipeline {

class BoundaryAnalyzer {
  public:
    explicit BoundaryAnalyzer(double min_contour_area = 400.0);

    RoadSegmentationResult analyze(const cv::Mat &mask, const cv::Size &frame_size,
                                   const cv::Rect &roi_rect,
                                   const std::string &segmentation_mode) const;

  private:
    double min_contour_area_{400.0};
};

} // namespace road_segmentation_lab::pipeline
