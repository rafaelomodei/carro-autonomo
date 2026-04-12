#include "services/vision/VisionDebugViewRenderer.hpp"

#include <algorithm>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::vision {
namespace {

namespace ts = autonomous_car::services::traffic_sign_detection;

const cv::Scalar kTrafficSignRoiColor{255, 200, 0};
const cv::Scalar kTrafficSignRawColor{0, 215, 255};
const cv::Scalar kTrafficSignCandidateColor{0, 165, 255};
const cv::Scalar kTrafficSignActiveColor{60, 220, 120};
const cv::Scalar kTrafficSignErrorColor{0, 0, 255};
const cv::Scalar kTrafficSignTextBg{16, 16, 16};

cv::Rect clampRectToFrame(const cv::Rect &rect, const cv::Size &size) {
    const cv::Rect bounds(0, 0, size.width, size.height);
    return rect & bounds;
}

cv::Size resolveTrafficSignSourceFrameSize(const ts::TrafficSignFrameResult &traffic_sign_result,
                                           const cv::Size &fallback_size) {
    return traffic_sign_result.roi.source_frame_size.area() > 0
               ? traffic_sign_result.roi.source_frame_size
               : fallback_size;
}

bool hasTrafficSignOverlayData(const ts::TrafficSignFrameResult &traffic_sign_result) {
    return traffic_sign_result.roi.source_frame_size.area() > 0 ||
           traffic_sign_result.roi.frame_rect.area() > 0 ||
           !traffic_sign_result.raw_detections.empty() ||
           traffic_sign_result.candidate.has_value() ||
           traffic_sign_result.active_detection.has_value() ||
           !traffic_sign_result.last_error.empty();
}

void drawLabelTag(cv::Mat &image, const cv::Point &anchor, const std::string &text,
                  const cv::Scalar &color) {
    const int baseline_y = std::clamp(anchor.y, 18, std::max(18, image.rows - 6));
    const cv::Size text_size =
        cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, nullptr);
    const cv::Rect background(anchor.x, baseline_y - text_size.height - 10,
                              text_size.width + 12, text_size.height + 10);
    const cv::Rect clipped = clampRectToFrame(background, image.size());
    cv::rectangle(image, clipped, kTrafficSignTextBg, cv::FILLED, cv::LINE_AA);
    cv::rectangle(image, clipped, color, 1, cv::LINE_AA);
    cv::putText(image, text,
                {clipped.x + 6, clipped.y + clipped.height - 6}, cv::FONT_HERSHEY_SIMPLEX,
                0.45, color, 1, cv::LINE_AA);
}

void drawDetectionBox(cv::Mat &image, const ts::TrafficSignDetection &detection,
                      const cv::Size &source_frame_size, const cv::Scalar &color, int thickness,
                      const std::string &prefix = {}) {
    ts::TrafficSignBoundingBox render_box = detection.bbox_frame;
    if (source_frame_size.area() > 0 && source_frame_size != image.size()) {
        render_box = ts::clampBoundingBox(
            ts::scaleBoundingBox(detection.bbox_frame, source_frame_size, image.size()),
            cv::Rect(0, 0, image.cols, image.rows));
    }

    const cv::Rect rect = clampRectToFrame(ts::toCvRect(render_box), image.size());
    if (rect.area() <= 0) {
        return;
    }

    cv::rectangle(image, rect, color, thickness, cv::LINE_AA);
    const std::string tag =
        prefix.empty()
            ? detection.display_label + " " +
                  cv::format("%.2f", detection.confidence_score)
            : prefix + ": " + detection.display_label + " " +
                  cv::format("%.2f", detection.confidence_score);
    drawLabelTag(image, {rect.x, std::max(18, rect.y - 4)}, tag, color);
}

void drawTrafficSignOverlay(cv::Mat &image, const ts::TrafficSignFrameResult &traffic_sign_result) {
    if (image.empty() || !hasTrafficSignOverlayData(traffic_sign_result)) {
        return;
    }

    const cv::Size source_frame_size =
        resolveTrafficSignSourceFrameSize(traffic_sign_result, image.size());

    if (traffic_sign_result.roi.debug_roi_enabled) {
        const ts::TrafficSignRoi render_roi = ts::buildTrafficSignRoi(
            image.size(), traffic_sign_result.roi.left_ratio, traffic_sign_result.roi.right_ratio,
            traffic_sign_result.roi.top_ratio, traffic_sign_result.roi.bottom_ratio,
            traffic_sign_result.roi.debug_roi_enabled);
        const cv::Rect roi_rect =
            clampRectToFrame(render_roi.frame_rect, image.size());
        if (roi_rect.area() > 0) {
            cv::rectangle(image, roi_rect, kTrafficSignRoiColor, 2, cv::LINE_AA);
            drawLabelTag(image, {roi_rect.x, roi_rect.y + 20}, "Traffic ROI",
                         kTrafficSignRoiColor);
        }
    }

    for (const auto &detection : traffic_sign_result.raw_detections) {
        drawDetectionBox(image, detection, source_frame_size, kTrafficSignRawColor, 1);
    }

    if (traffic_sign_result.candidate.has_value()) {
        drawDetectionBox(image, *traffic_sign_result.candidate, source_frame_size,
                         kTrafficSignCandidateColor, 2,
                         "Candidate");
    }

    if (traffic_sign_result.active_detection.has_value()) {
        drawDetectionBox(image, *traffic_sign_result.active_detection, source_frame_size,
                         kTrafficSignActiveColor, 3,
                         "Active");
    }

    if (traffic_sign_result.detector_state == ts::TrafficSignDetectorState::Error &&
        !traffic_sign_result.last_error.empty()) {
        drawLabelTag(image, {12, image.rows - 12}, "Erro sinalizacao",
                     kTrafficSignErrorColor);
    }
}

} // namespace

cv::Mat VisionDebugViewRenderer::render(
    VisionDebugViewId view, const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    const road_segmentation_lab::config::LabConfig &config, const std::string &source_label,
    const std::string &calibration_status,
    const autonomous_car::services::autonomous_control::AutonomousControlSnapshot
        &snapshot,
    const VisionRuntimeTelemetry &runtime_telemetry,
    const autonomous_car::services::traffic_sign_detection::TrafficSignFrameResult
        &traffic_sign_result) const {
    switch (view) {
    case VisionDebugViewId::Raw:
        return segmentation_renderer_.renderRawView(result);
    case VisionDebugViewId::Preprocess:
        return segmentation_renderer_.renderPreprocessView(result);
    case VisionDebugViewId::Mask:
        return segmentation_renderer_.renderMaskView(result);
    case VisionDebugViewId::Annotated: {
        cv::Mat annotated = segmentation_renderer_.renderAnnotatedView(result);
        drawTrafficSignOverlay(annotated, traffic_sign_result);
        return annotated;
    }
    case VisionDebugViewId::Dashboard:
        return dashboard_renderer_.render(result, config, source_label, calibration_status,
                                          snapshot, runtime_telemetry, traffic_sign_result);
    }

    return cv::Mat();
}

} // namespace autonomous_car::services::vision
