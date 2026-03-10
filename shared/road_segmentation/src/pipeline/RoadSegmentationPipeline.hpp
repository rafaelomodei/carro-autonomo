#pragma once

#include <opencv2/core.hpp>

#include <string>

#include "config/LabConfig.hpp"
#include "pipeline/BoundaryAnalyzer.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "pipeline/stages/GaussianStage.hpp"
#include "pipeline/stages/IlluminationStage.hpp"
#include "pipeline/stages/MorphologyStage.hpp"
#include "pipeline/stages/ResizeStage.hpp"
#include "pipeline/stages/RoiStage.hpp"
#include "pipeline/stages/SegmentationStage.hpp"
#include "pipeline/stages/UndistortStage.hpp"

namespace road_segmentation_lab::pipeline {

class RoadSegmentationPipeline {
  public:
    RoadSegmentationPipeline();
    explicit RoadSegmentationPipeline(const config::LabConfig &config);

    void updateConfig(const config::LabConfig &config);
    const config::LabConfig &config() const noexcept;
    std::string calibrationStatus() const;

    RoadSegmentationResult process(const cv::Mat &frame) const;

  private:
    config::LabConfig config_;
    stages::ResizeStage resize_stage_{true, {320, 240}};
    stages::UndistortStage undistort_stage_;
    stages::RoiStage roi_stage_{0.5, 1.0};
    stages::GaussianStage gaussian_stage_{true, {5, 5}, 1.2};
    stages::IlluminationStage illumination_stage_{false};
    stages::SegmentationStage segmentation_stage_{config_};
    stages::MorphologyStage morphology_stage_{true, {5, 5}, 1};
    BoundaryAnalyzer boundary_analyzer_{400.0};
};

} // namespace road_segmentation_lab::pipeline
