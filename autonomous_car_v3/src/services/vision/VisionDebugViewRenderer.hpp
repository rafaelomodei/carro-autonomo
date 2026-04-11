#pragma once

#include <opencv2/core.hpp>

#include <string>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/autonomous_control/AutonomousControlDebugRenderer.hpp"
#include "services/autonomous_control/AutonomousControlTypes.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"
#include "services/vision/VisionDebugStream.hpp"
#include "render/DebugRenderer.hpp"

namespace autonomous_car::services::vision {

class VisionDebugViewRenderer {
public:
    cv::Mat render(VisionDebugViewId view,
                   const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
                   const road_segmentation_lab::config::LabConfig &config,
                   const std::string &source_label,
                   const std::string &calibration_status,
                   const autonomous_car::services::autonomous_control::AutonomousControlSnapshot
                       &snapshot,
                   const autonomous_car::services::traffic_sign_detection::TrafficSignFrameResult
                       &traffic_sign_result) const;

private:
    road_segmentation_lab::render::DebugRenderer segmentation_renderer_;
    autonomous_car::services::autonomous_control::AutonomousControlDebugRenderer
        dashboard_renderer_;
};

} // namespace autonomous_car::services::vision
