#include "services/camera/LaneVisualizer.hpp"

#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera {
namespace {
const cv::Scalar kLeftColor{255, 215, 0};
const cv::Scalar kRightColor{0, 215, 255};
const cv::Scalar kCenterColor{0, 255, 0};
const cv::Scalar kFrameCenterColor{255, 255, 255};
const cv::Scalar kEvaColor{0, 255, 0};
const cv::Scalar kLeftPaperColor{0, 0, 255};
const cv::Scalar kRightPaperColor{255, 0, 0};
constexpr double kMinContourArea = 500.0;

struct RegionMasks {
    cv::Mat eva;
    cv::Mat left;
    cv::Mat right;
};

cv::Mat ToBinaryMask(const cv::Mat &mask) {
    if (mask.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (mask.channels() == 1) {
        gray = mask;
    } else {
        cv::cvtColor(mask, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY);
    return binary;
}

RegionMasks BuildRegionMasks(const cv::Mat &white_mask, const cv::Mat &roi_mask) {
    RegionMasks masks;
    if (white_mask.empty()) {
        return masks;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(white_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const int width = white_mask.cols;
    masks.left = cv::Mat::zeros(white_mask.size(), CV_8U);
    masks.right = cv::Mat::zeros(white_mask.size(), CV_8U);

    for (const auto &contour : contours) {
        if (cv::contourArea(contour) < kMinContourArea) {
            continue;
        }

        const cv::Moments m = cv::moments(contour);
        if (m.m00 == 0.0) {
            continue;
        }

        const double cx = m.m10 / m.m00;
        if (cx < static_cast<double>(width) / 2.0) {
            cv::drawContours(masks.left, std::vector<std::vector<cv::Point>>{contour}, -1, 255,
                             cv::FILLED);
        } else {
            cv::drawContours(masks.right, std::vector<std::vector<cv::Point>>{contour}, -1, 255,
                             cv::FILLED);
        }
    }

    cv::bitwise_not(white_mask, masks.eva);
    if (!roi_mask.empty()) {
        cv::bitwise_and(masks.eva, roi_mask, masks.eva);
    }

    return masks;
}

cv::Mat BuildColoredMask(const RegionMasks &masks) {
    if (masks.eva.empty() && masks.left.empty() && masks.right.empty()) {
        return cv::Mat();
    }

    const cv::Size size = !masks.eva.empty()
                               ? masks.eva.size()
                               : (!masks.left.empty() ? masks.left.size() : masks.right.size());

    cv::Mat colored = cv::Mat::zeros(size, CV_8UC3);
    if (!masks.eva.empty()) {
        colored.setTo(kEvaColor, masks.eva);
    }
    if (!masks.left.empty()) {
        colored.setTo(kLeftPaperColor, masks.left);
    }
    if (!masks.right.empty()) {
        colored.setTo(kRightPaperColor, masks.right);
    }

    return colored;
}

cv::Mat BuildCombinedMask(const RegionMasks &masks) {
    cv::Mat combined;
    if (!masks.eva.empty()) {
        combined = masks.eva.clone();
    }

    if (!masks.left.empty()) {
        if (combined.empty()) {
            combined = masks.left.clone();
        } else {
            cv::bitwise_or(combined, masks.left, combined);
        }
    }

    if (!masks.right.empty()) {
        if (combined.empty()) {
            combined = masks.right.clone();
        } else {
            cv::bitwise_or(combined, masks.right, combined);
        }
    }

    return combined;
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

    const cv::Mat binary_mask = ToBinaryMask(result.processed_frame);
    const RegionMasks region_masks = BuildRegionMasks(binary_mask, result.roi_mask);
    const cv::Mat colored_mask = BuildColoredMask(region_masks);
    const cv::Mat combined_mask = BuildCombinedMask(region_masks);

    cv::Mat overlay = applyMaskOverlay(frame, colored_mask, combined_mask);
    drawLaneOverlay(overlay, result);

    cv::Mat mask_view = colored_mask.empty() ? cv::Mat::zeros(frame.size(), frame.type())
                                             : colored_mask;

    cv::putText(mask_view, "Mascara (HSV + filtros)", {20, 40}, cv::FONT_HERSHEY_SIMPLEX,
                0.8, {255, 255, 255}, 2);

    cv::Mat mask_resized;
    cv::resize(mask_view, mask_resized, overlay.size());

    cv::Mat debug_view(overlay.rows, overlay.cols * 2, overlay.type());
    overlay.copyTo(debug_view(cv::Rect(0, 0, overlay.cols, overlay.rows)));
    mask_resized.copyTo(debug_view(cv::Rect(overlay.cols, 0, overlay.cols, overlay.rows)));

    return debug_view;
}

cv::Mat LaneVisualizer::toBinaryMask(const cv::Mat &mask) const {
    return ToBinaryMask(mask);
}

cv::Mat LaneVisualizer::buildColoredMask(const cv::Mat &binary_mask) const {
    const RegionMasks masks = BuildRegionMasks(binary_mask, {});
    return BuildColoredMask(masks);
}

cv::Mat LaneVisualizer::applyMaskOverlay(const cv::Mat &frame, const cv::Mat &colored_mask,
                                         const cv::Mat &binary_mask) const {
    if (frame.empty() || colored_mask.empty() || binary_mask.empty()) {
        return frame.clone();
    }

    cv::Mat overlay = frame.clone();
    colored_mask.copyTo(overlay, binary_mask);

    cv::Mat blended;
    cv::addWeighted(overlay, 0.45, frame, 0.55, 0.0, blended);
    return blended;
}

} // namespace autonomous_car::services::camera
