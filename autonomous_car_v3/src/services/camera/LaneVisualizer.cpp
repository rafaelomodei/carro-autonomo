#include "services/camera/LaneVisualizer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera {
namespace {
const cv::Scalar kLeftColor{255, 215, 0};
const cv::Scalar kRightColor{0, 215, 255};
const cv::Scalar kCenterColor{0, 255, 0};
const cv::Scalar kFrameCenterColor{255, 255, 255};
const cv::Scalar kHorizonColor{180, 180, 255};

cv::Point ProjectPointToY(const cv::Point &a, const cv::Point &b, int target_y, int image_width) {
    if (image_width <= 0) {
        return {};
    }

    const int clamped_y = std::clamp(target_y, 0, std::numeric_limits<int>::max());
    if (a.y == b.y) {
        return {std::clamp(a.x, 0, image_width - 1), clamped_y};
    }

    const double slope = static_cast<double>(b.x - a.x) / static_cast<double>(b.y - a.y);
    const double x = static_cast<double>(a.x) + slope * static_cast<double>(clamped_y - a.y);
    return {std::clamp(static_cast<int>(std::lround(x)), 0, image_width - 1), clamped_y};
}

LaneBoundarySegment AlignBoundaryToHorizon(const LaneBoundarySegment &boundary, int horizon_y,
                                           int image_width) {
    LaneBoundarySegment aligned;
    if (!boundary.valid) {
        return aligned;
    }

    const cv::Point top = ProjectPointToY(boundary.top, boundary.bottom, horizon_y, image_width);
    const int bottom_y = std::max(boundary.bottom.y, top.y + 1);
    const cv::Point bottom = ProjectPointToY(boundary.top, boundary.bottom, bottom_y, image_width);

    aligned.top = top;
    aligned.bottom = bottom;
    aligned.valid = true;
    return aligned;
}
} // namespace

void LaneVisualizer::drawLaneOverlay(cv::Mat &frame, const LaneDetectionResult &result) const {
    if (frame.empty()) {
        return;
    }

    cv::line(frame, {result.frame_center.x, 0},
             {result.frame_center.x, frame.rows}, kFrameCenterColor, 1, cv::LINE_AA);

    const cv::Point horizon_point = computeHorizonPoint(result, frame.rows);

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

    const auto aligned_left = AlignBoundaryToHorizon(result.left_boundary, horizon_point.y, frame.cols);
    const auto aligned_right =
        AlignBoundaryToHorizon(result.right_boundary, horizon_point.y, frame.cols);

    drawBoundary(aligned_left, kLeftColor);
    drawBoundary(aligned_right, kRightColor);

    cv::circle(frame, result.lane_center, 6, kCenterColor, cv::FILLED);
    cv::circle(frame, horizon_point, 5, kHorizonColor, cv::FILLED);
    cv::line(frame, horizon_point, result.lane_center, kHorizonColor, 2, cv::LINE_AA);
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

    cv::Mat binary_mask = toBinaryMask(result.processed_frame);
    cv::Mat colored_mask = buildColoredMask(binary_mask);

    cv::Mat overlay = applyMaskOverlay(frame, colored_mask, binary_mask);
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

cv::Point LaneVisualizer::computeHorizonPoint(const LaneDetectionResult &result, int image_height) const {
    if (image_height <= 0) {
        return result.frame_center;
    }

    const int default_y = image_height / 3;
    int horizon_y = default_y;

    if (result.left_boundary.valid && result.right_boundary.valid) {
        horizon_y = std::min(result.left_boundary.top.y, result.right_boundary.top.y);
    } else if (result.left_boundary.valid) {
        horizon_y = result.left_boundary.top.y;
    } else if (result.right_boundary.valid) {
        horizon_y = result.right_boundary.top.y;
    }

    horizon_y = std::clamp(horizon_y, 0, image_height - 1);
    const int max_width = std::max(0, result.frame_center.x * 2 - 1);
    const int horizon_x = result.lane_center.x > 0 ? result.lane_center.x : result.frame_center.x;
    return {std::clamp(horizon_x, 0, max_width), horizon_y};
}

cv::Mat LaneVisualizer::toBinaryMask(const cv::Mat &mask) const {
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

cv::Mat LaneVisualizer::buildColoredMask(const cv::Mat &binary_mask) const {
    if (binary_mask.empty()) {
        return cv::Mat();
    }

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int components =
        cv::connectedComponentsWithStats(binary_mask, labels, stats, centroids, 8, CV_32S);

    if (components <= 1) {
        return cv::Mat();
    }

    std::vector<cv::Vec3b> colors(static_cast<size_t>(components));
    colors[0] = {0, 0, 0};

    for (int i = 1; i < components; ++i) {
        const unsigned seed = static_cast<unsigned>(i * 73856093u) ^ 0x5bd1e995u;
        colors[static_cast<size_t>(i)] =
            cv::Vec3b(seed & 0xFFu, (seed >> 8u) & 0xFFu, (seed >> 16u) & 0xFFu);
    }

    cv::Mat colored(binary_mask.size(), CV_8UC3);
    for (int y = 0; y < labels.rows; ++y) {
        const int *label_row = labels.ptr<int>(y);
        cv::Vec3b *color_row = colored.ptr<cv::Vec3b>(y);
        for (int x = 0; x < labels.cols; ++x) {
            const int label = label_row[x];
            color_row[x] = colors[static_cast<size_t>(label)];
        }
    }

    return colored;
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
