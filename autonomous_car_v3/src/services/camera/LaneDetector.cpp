#include "services/camera/LaneDetector.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <opencv2/imgproc.hpp>

#include "services/camera/filters/DarkRegionMaskFilter.hpp"
#include "services/camera/filters/GaussianBlurFilter.hpp"
#include "services/camera/filters/RoiBandFilter.hpp"
#include "services/camera/filters/MorphologyFilter.hpp"

namespace autonomous_car::services::camera {
namespace {
std::vector<std::unique_ptr<filters::FrameFilter>> BuildFilters(const LaneFilterConfig &config) {
    std::vector<std::unique_ptr<filters::FrameFilter>> filters;
    filters.push_back(
        std::make_unique<filters::RoiBandFilter>(config.roi_band_start_ratio, config.roi_band_end_ratio));
    filters.push_back(
        std::make_unique<filters::GaussianBlurFilter>(config.gaussian_kernel, config.gaussian_sigma));
    filters.push_back(
        std::make_unique<filters::DarkRegionMaskFilter>(config.hsv_low, config.hsv_high));
    filters.push_back(std::make_unique<filters::MorphologyFilter>(cv::MORPH_CLOSE, config.morph_kernel,
                                                                 config.morph_iterations));
    return filters;
}

struct BoundarySamples {
    std::vector<cv::Point> left;
    std::vector<cv::Point> right;
};

BoundarySamples CollectBoundarySamples(const cv::Mat &mask, const cv::Rect &bounds, int center_x) {
    BoundarySamples samples;
    if (mask.empty()) {
        return samples;
    }

    const int width = mask.cols;
    if (width <= 0) {
        return samples;
    }

    center_x = std::clamp(center_x, 0, width - 1);
    const int y_start = std::clamp(bounds.y, 0, mask.rows);
    const int y_end = std::clamp(bounds.y + bounds.height, 0, mask.rows);

    for (int y = y_start; y < y_end; ++y) {
        const auto *row = mask.ptr<uchar>(y);
        if (row[center_x] == 0) {
            continue;
        }

        int left = center_x;
        while (left > 0 && row[left] > 0) {
            --left;
        }
        if (left < center_x) {
            samples.left.emplace_back(left + 1, y);
        }

        int right = center_x;
        while (right < width - 1 && row[right] > 0) {
            ++right;
        }
        if (right > center_x) {
            samples.right.emplace_back(right - 1, y);
        }
    }

    return samples;
}

LaneBoundarySegment FitBoundarySegment(const std::vector<cv::Point> &points, int image_width) {
    LaneBoundarySegment boundary;
    if (points.size() < 2 || image_width <= 0) {
        return boundary;
    }

    cv::Vec4f line;
    cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);
    const float vx = line[0];
    const float vy = line[1];
    const float x0 = line[2];
    const float y0 = line[3];

    const auto [min_it, max_it] = std::minmax_element(
        points.begin(), points.end(),
        [](const cv::Point &lhs, const cv::Point &rhs) { return lhs.y < rhs.y; });
    const int top_y = min_it->y;
    const int bottom_y = max_it->y;

    const auto compute_x = [&](int y) {
        if (std::abs(vy) < 1e-5f) {
            return std::clamp(static_cast<int>(std::lround(x0)), 0, image_width - 1);
        }
        const float x = x0 + (vx / vy) * (static_cast<float>(y) - y0);
        return std::clamp(static_cast<int>(std::lround(x)), 0, image_width - 1);
    };

    boundary.top = {compute_x(top_y), top_y};
    boundary.bottom = {compute_x(bottom_y), bottom_y};
    boundary.valid = true;
    return boundary;
}

LaneBoundarySegment BuildVerticalFallback(int x, int top_y, int bottom_y, int width) {
    LaneBoundarySegment boundary;
    if (width <= 0 || x < 0 || x >= width) {
        return boundary;
    }

    top_y = std::max(0, std::min(top_y, bottom_y));
    bottom_y = std::max(top_y, bottom_y);
    boundary.top = {x, top_y};
    boundary.bottom = {x, bottom_y};
    boundary.valid = true;
    return boundary;
}
}

LaneDetector::LaneDetector() : LaneDetector(LaneFilterConfig::LoadFromEnv()) {}

LaneDetector::LaneDetector(LaneFilterConfig config)
    : config_(std::move(config)), filters_(BuildFilters(config_)) {}

LaneDetectionResult LaneDetector::detect(const cv::Mat &frame) const {
    LaneDetectionResult result;
    if (frame.empty()) {
        return result;
    }

    cv::Mat processed = frame;
    for (const auto &filter : filters_) {
        processed = filter->apply(processed);
    }

    if (processed.empty()) {
        return result;
    }

    result.processed_frame = processed;
    result.frame_center = cv::Point(frame.cols / 2, frame.rows - 1);

    LaneDetectionResult analysis = analyzeMask(processed, frame.size());
    analysis.processed_frame = result.processed_frame;
    analysis.frame_center = result.frame_center;
    return analysis;
}

LaneDetectionResult LaneDetector::analyzeMask(const cv::Mat &mask,
                                              const cv::Size &frame_size) const {
    LaneDetectionResult result;
    result.frame_center = cv::Point(frame_size.width / 2, frame_size.height - 1);
    const int default_horizon =
        std::clamp(static_cast<int>(std::round(frame_size.height * config_.roi_band_start_ratio)), 0,
                   std::max(0, frame_size.height - 1));
    result.horizon_point = {result.frame_center.x, default_horizon};

    if (mask.empty()) {
        return result;
    }

    cv::Mat single_channel;
    if (mask.channels() > 1) {
        cv::cvtColor(mask, single_channel, cv::COLOR_BGR2GRAY);
    } else {
        single_channel = mask;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(single_channel, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        return result;
    }

    const auto largest_it = std::max_element(
        contours.begin(), contours.end(),
        [](const auto &lhs, const auto &rhs) { return cv::contourArea(lhs) < cv::contourArea(rhs); });
    const auto &largest_contour = *largest_it;
    if (cv::contourArea(largest_contour) < config_.min_contour_area) {
        return result;
    }

    const int bottom_row = single_channel.rows - 1;
    const int band_height = std::max(5, single_channel.rows / 12);
    const int band_start = std::max(0, bottom_row - band_height);

    int fallback_left_x = single_channel.cols;
    int fallback_right_x = -1;
    for (const auto &point : largest_contour) {
        if (point.y >= band_start) {
            fallback_left_x = std::min(fallback_left_x, point.x);
            fallback_right_x = std::max(fallback_right_x, point.x);
        }
    }

    const cv::Rect bounds = cv::boundingRect(largest_contour);
    if (fallback_left_x > fallback_right_x) {
        fallback_left_x = bounds.x;
        fallback_right_x = bounds.x + bounds.width;
    }

    result.horizon_point.y = std::clamp(bounds.y, 0, std::max(0, frame_size.height - 1));
    result.horizon_found = true;

    const cv::Moments contour_moments = cv::moments(largest_contour);
    if (contour_moments.m00 != 0.0) {
        result.lane_center = cv::Point(static_cast<int>(contour_moments.m10 / contour_moments.m00),
                                       static_cast<int>(contour_moments.m01 / contour_moments.m00));
    } else if (fallback_left_x <= fallback_right_x) {
        result.lane_center =
            cv::Point((fallback_left_x + fallback_right_x) / 2, bottom_row - band_height / 2);
    } else {
        result.lane_center = cv::Point(result.frame_center.x, bottom_row - band_height / 2);
    }

    result.horizon_point.x = result.lane_center.x;

    const auto samples = CollectBoundarySamples(single_channel, bounds, result.lane_center.x);
    result.left_boundary = FitBoundarySegment(samples.left, single_channel.cols);
    if (!result.left_boundary.valid && fallback_left_x < single_channel.cols) {
        result.left_boundary = BuildVerticalFallback(fallback_left_x, band_start, bottom_row,
                                                     single_channel.cols);
    }

    result.right_boundary = FitBoundarySegment(samples.right, single_channel.cols);
    if (!result.right_boundary.valid && fallback_right_x >= 0 &&
        fallback_right_x < single_channel.cols) {
        result.right_boundary = BuildVerticalFallback(fallback_right_x, band_start, bottom_row,
                                                      single_channel.cols);
    }

    result.lane_found = result.left_boundary.valid && result.right_boundary.valid;
    if (!result.lane_found) {
        return result;
    }

    result.lateral_offset_px = static_cast<double>(result.lane_center.x - result.frame_center.x);
    if (frame_size.width > 0) {
        result.lateral_offset_percentage =
            (result.lateral_offset_px / (static_cast<double>(frame_size.width) / 2.0)) * 100.0;
    }

    return result;
}

} // namespace autonomous_car::services::camera
