#include "services/camera/LaneVisualizer.hpp"

#include <algorithm>
#include <cmath>
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

cv::Point ProjectToY(const LaneBoundarySegment &boundary, int target_y, int width, int height) {
    target_y = std::clamp(target_y, 0, std::max(0, height - 1));
    if (!boundary.valid || width <= 0 || height <= 0) {
        return {};
    }

    const int x1 = boundary.top.x;
    const int y1 = boundary.top.y;
    const int x2 = boundary.bottom.x;
    const int y2 = boundary.bottom.y;

    if (y1 == y2) {
        const int clamped_x = std::clamp(x1, 0, std::max(0, width - 1));
        return {clamped_x, target_y};
    }

    const float t = static_cast<float>(target_y - y1) / static_cast<float>(y2 - y1);
    const int x = std::clamp(static_cast<int>(std::lround(x1 + (x2 - x1) * t)), 0,
                              std::max(0, width - 1));
    return {x, target_y};
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

    const bool has_horizon =
        result.horizon_found && result.horizon_point.y >= 0 && result.horizon_point.y < frame.rows;
    const cv::Point horizon = has_horizon
                                  ? cv::Point(std::clamp(result.horizon_point.x, 0, frame.cols - 1),
                                              std::clamp(result.horizon_point.y, 0, frame.rows - 1))
                                  : cv::Point();

    const auto drawBoundary = [&](const LaneBoundarySegment &boundary, const cv::Scalar &color) {
        if (!boundary.valid) {
            return;
        }
        cv::Point start = boundary.top;
        if (has_horizon) {
            start = ProjectToY(boundary, horizon.y, frame.cols, frame.rows);
        }
        cv::line(frame, start, boundary.bottom, color, 2, cv::LINE_AA);
    };

    drawBoundary(result.left_boundary, kLeftColor);
    drawBoundary(result.right_boundary, kRightColor);

    if (has_horizon) {
        cv::circle(frame, horizon, 4, kFrameCenterColor, cv::FILLED);
    }

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
