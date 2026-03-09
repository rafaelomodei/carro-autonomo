#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationResult.hpp"

namespace road_segmentation_lab::render {

class DebugRenderer {
  public:
    cv::Mat render(const pipeline::RoadSegmentationResult &result,
                   const config::LabConfig &config, const std::string &source_label,
                   const std::string &calibration_status) const;

  private:
    cv::Mat buildOriginalTile(const pipeline::RoadSegmentationResult &result,
                              const std::string &source_label) const;
    cv::Mat buildPreprocessTile(const pipeline::RoadSegmentationResult &result,
                                const config::LabConfig &config,
                                const std::string &calibration_status) const;
    cv::Mat buildMaskTile(const pipeline::RoadSegmentationResult &result) const;
    cv::Mat buildOutputTile(const pipeline::RoadSegmentationResult &result) const;

    static cv::Mat ensureColor(const cv::Mat &frame);
    static void drawLabelBlock(cv::Mat &image, const std::string &title,
                               const std::vector<std::string> &lines);
};

} // namespace road_segmentation_lab::render
