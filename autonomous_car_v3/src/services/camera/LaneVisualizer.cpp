#include "services/camera/LaneVisualizer.hpp"

#include <iomanip>
#include <sstream>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera {
namespace {
const cv::Scalar kLeftColor{255, 215, 0};
const cv::Scalar kRightColor{0, 215, 255};
const cv::Scalar kCenterColor{0, 255, 0};
const cv::Scalar kFrameCenterColor{255, 255, 255};
const cv::Scalar kRoadColor{0, 0, 255};

void paintMask(cv::Mat &target, const cv::Mat &mask, const cv::Scalar &color) {
    if (target.empty() || mask.empty()) {
        return;
    }
    cv::Mat colored(target.size(), target.type(), color);
    colored.copyTo(target, mask);
}
}

void LaneVisualizer::drawLaneOverlay(cv::Mat &frame, const LaneDetectionResult &result) const {
    if (frame.empty()) {
        return;
    }

    cv::line(frame, {result.frame_center.x, 0},
             {result.frame_center.x, frame.rows}, kFrameCenterColor, 1, cv::LINE_AA);

    if (!result.lane_found) {
        cv::putText(frame, "Faixa nao detectada", {20, 40}, cv::FONT_HERSHEY_SIMPLEX,
                    0.9, {0, 0, 255}, 2);
        return;
    }

    const auto drawBoundary = [&](const LaneBoundarySegment &boundary, const cv::Scalar &color) {
        if (!boundary.valid) {
            return;
        }
        cv::line(frame, boundary.top, boundary.bottom, color, 2, cv::LINE_AA);
    };

    drawBoundary(result.left_boundary, kLeftColor);
    drawBoundary(result.right_boundary, kRightColor);

    cv::circle(frame, result.lane_center, 6, kCenterColor, cv::FILLED);
    cv::line(frame, result.frame_center, result.lane_center, kCenterColor, 2, cv::LINE_AA);

    std::ostringstream offset_text;
    offset_text << std::fixed << std::setprecision(1)
                << "Offset: " << result.lateral_offset_px << "px ("
                << result.lateral_offset_percentage << "%)";
    cv::putText(frame, offset_text.str(), {20, frame.rows - 20}, cv::FONT_HERSHEY_SIMPLEX,
                0.7, {255, 255, 255}, 2);
}

cv::Mat LaneVisualizer::buildDebugView(const cv::Mat &frame, const LaneDetectionResult &result) const {
    if (frame.empty()) {
        return frame;
    }

    cv::Mat overlay = frame.clone();
    paintMask(overlay, result.road_mask, kRoadColor);
    paintMask(overlay, result.left_mask, kLeftColor);
    paintMask(overlay, result.right_mask, kRightColor);
    cv::addWeighted(overlay, 0.45, frame, 0.55, 0.0, overlay);
    drawLaneOverlay(overlay, result);

    cv::Mat mask_view = cv::Mat::zeros(frame.size(), frame.type());
    paintMask(mask_view, result.road_mask, kRoadColor);
    paintMask(mask_view, result.left_mask, kLeftColor);
    paintMask(mask_view, result.right_mask, kRightColor);

    cv::putText(mask_view, "Segmentacao de pista", {20, 40}, cv::FONT_HERSHEY_SIMPLEX, 0.8,
                {255, 255, 255}, 2);

    cv::Mat mask_resized;
    cv::resize(mask_view, mask_resized, overlay.size());

    cv::Mat debug_view(overlay.rows, overlay.cols * 2, overlay.type());
    overlay.copyTo(debug_view(cv::Rect(0, 0, overlay.cols, overlay.rows)));
    mask_resized.copyTo(debug_view(cv::Rect(overlay.cols, 0, overlay.cols, overlay.rows)));

    return debug_view;
}

} // namespace autonomous_car::services::camera
