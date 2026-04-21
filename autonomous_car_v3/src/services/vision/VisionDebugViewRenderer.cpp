#include "services/vision/VisionDebugViewRenderer.hpp"

namespace autonomous_car::services::vision {

cv::Mat VisionDebugViewRenderer::render(
    VisionDebugViewId view, const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    const road_segmentation_lab::config::LabConfig &config, const std::string &source_label,
    const std::string &calibration_status,
    const autonomous_car::services::autonomous_control::AutonomousControlSnapshot &snapshot,
    const VisionRuntimeTelemetry &runtime_telemetry) const {
    switch (view) {
    case VisionDebugViewId::Raw:
        return segmentation_renderer_.renderRawView(result);
    case VisionDebugViewId::Preprocess:
        return segmentation_renderer_.renderPreprocessView(result);
    case VisionDebugViewId::Mask:
        return segmentation_renderer_.renderMaskView(result);
    case VisionDebugViewId::Annotated:
        return segmentation_renderer_.renderAnnotatedView(result);
    case VisionDebugViewId::Dashboard:
        return dashboard_renderer_.render(result, config, source_label, calibration_status,
                                          snapshot, runtime_telemetry);
    }

    return cv::Mat();
}

} // namespace autonomous_car::services::vision
