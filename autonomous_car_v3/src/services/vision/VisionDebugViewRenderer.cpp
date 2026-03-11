#include "services/vision/VisionDebugViewRenderer.hpp"

#include <cmath>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::vision {
namespace {

void drawTrafficSignOverlay(
    cv::Mat &frame,
    const autonomous_car::services::traffic_sign_detection::TrafficSignFrameResult &traffic_signs,
    const cv::Point &offset = {0, 0}) {
    namespace ts = autonomous_car::services::traffic_sign_detection;

    if (frame.empty() || !traffic_signs.enabled) {
        return;
    }

    const cv::Rect roi_rect(traffic_signs.roi.x + offset.x, traffic_signs.roi.y + offset.y,
                            traffic_signs.roi.width, traffic_signs.roi.height);
    if (roi_rect.area() > 0) {
        cv::rectangle(frame, roi_rect, {255, 160, 0}, 2, cv::LINE_AA);
    }

    auto draw_detection =
        [&](const ts::TrafficSignDetection &detection, const cv::Scalar &color,
            int thickness) {
            const cv::Rect box(detection.bbox_frame.x + offset.x,
                               detection.bbox_frame.y + offset.y,
                               detection.bbox_frame.width, detection.bbox_frame.height);
            if (box.area() <= 0) {
                return;
            }

            cv::rectangle(frame, box, color, thickness, cv::LINE_AA);
            const std::string label =
                detection.display_label + " " +
                std::to_string(static_cast<int>(std::lround(detection.confidence_score * 100.0))) +
                "%";
            const cv::Point label_origin(box.x, std::max(18, box.y - 8));
            cv::putText(frame, label, label_origin, cv::FONT_HERSHEY_SIMPLEX, 0.45, color, 1,
                        cv::LINE_AA);
        };

    for (const auto &detection : traffic_signs.raw_detections) {
        draw_detection(detection, {60, 220, 255}, 2);
    }

    if (traffic_signs.active_detection.has_value()) {
        draw_detection(*traffic_signs.active_detection, {80, 240, 120}, 3);
    } else if (traffic_signs.candidate.has_value()) {
        draw_detection(*traffic_signs.candidate, {0, 180, 255}, 2);
    }
}

} // namespace

cv::Mat VisionDebugViewRenderer::render(
    VisionDebugViewId view, const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    const road_segmentation_lab::config::LabConfig &config, const std::string &source_label,
    const std::string &calibration_status,
    const autonomous_car::services::autonomous_control::AutonomousControlSnapshot
        &snapshot,
    const autonomous_car::services::traffic_sign_detection::TrafficSignFrameResult
        &traffic_signs) const {
    switch (view) {
    case VisionDebugViewId::Raw:
        return segmentation_renderer_.renderRawView(result);
    case VisionDebugViewId::Preprocess:
        return segmentation_renderer_.renderPreprocessView(result);
    case VisionDebugViewId::Mask:
        return segmentation_renderer_.renderMaskView(result);
    case VisionDebugViewId::Annotated: {
        cv::Mat annotated = segmentation_renderer_.renderAnnotatedView(result);
        drawTrafficSignOverlay(annotated, traffic_signs);
        return annotated;
    }
    case VisionDebugViewId::Dashboard:
        return dashboard_renderer_.render(result, config, source_label, calibration_status,
                                          snapshot, traffic_signs);
    }

    return cv::Mat();
}

} // namespace autonomous_car::services::vision
