#pragma once

#include <opencv2/core.hpp>

#include <string>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "render/DebugRenderer.hpp"
#include "services/autonomous_control/AutonomousControlTypes.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace autonomous_car::services::autonomous_control {

class AutonomousControlDebugRenderer {
public:
    cv::Mat render(const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
                   const road_segmentation_lab::config::LabConfig &config,
                   const std::string &source_label,
                   const std::string &calibration_status,
                   const AutonomousControlSnapshot &snapshot,
                   const autonomous_car::services::vision::VisionRuntimeTelemetry
                       &runtime_telemetry) const;

private:
    static cv::Mat buildControlPanel(
        const AutonomousControlSnapshot &snapshot,
        const autonomous_car::services::vision::VisionRuntimeTelemetry &runtime_telemetry,
        cv::Size size);
    static void drawControlStatus(cv::Mat &panel, const AutonomousControlSnapshot &snapshot,
                                  const cv::Rect &area);
    static void drawRuntimeStatus(
        cv::Mat &panel,
        const autonomous_car::services::vision::VisionRuntimeTelemetry &runtime_telemetry,
        const cv::Rect &area);
    static void drawSteeringGauge(cv::Mat &panel, const AutonomousControlSnapshot &snapshot,
                                  const cv::Rect &area);
    static void drawTopDownPreview(cv::Mat &panel, const AutonomousControlSnapshot &snapshot,
                                   const cv::Rect &area);
    static int mapNormalizedX(double x, const cv::Rect &area);
    static int mapNormalizedY(double y, const cv::Rect &area);
    static std::string formatDouble(double value, int precision = 3);

    road_segmentation_lab::render::DebugRenderer segmentation_renderer_;
};

} // namespace autonomous_car::services::autonomous_control
