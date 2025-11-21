#include "services/camera/LaneDetector.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
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
    if (frame.empty() || frame.rows <= 0 || frame.cols <= 0) {
        return result;
    }

    cv::Mat processed = frame;
    for (const auto &filter : filters_) {
        processed = filter->apply(processed);
    }

    return analyzeMask(processed, frame.size());
}

LaneDetectionResult LaneDetector::analyzeMask(const cv::Mat &mask,
                                              const cv::Size &frame_size) const {
    LaneDetectionResult result;
    result.frame_center = cv::Point(frame_size.width / 2, frame_size.height - 1);

    if (mask.empty()) {
        return result;
    }

    cv::Mat single_channel;
    if (mask.channels() > 1) {
        cv::cvtColor(mask, single_channel, cv::COLOR_BGR2GRAY);
    } else {
        single_channel = mask;
    }

    const int total_rows = single_channel.rows;
    int roi_start = std::clamp(static_cast<int>(std::round(total_rows * config_.roi_band_start_ratio)), 0,
                               total_rows);
    int roi_end = std::clamp(static_cast<int>(std::round(total_rows * config_.roi_band_end_ratio)), 0,
                             total_rows);
    if (roi_end <= roi_start) {
        roi_start = 0;
        roi_end = total_rows;
    }
    cv::Mat band_mask = cv::Mat::zeros(single_channel.size(), single_channel.type());
    band_mask(cv::Range(roi_start, roi_end), cv::Range::all()).setTo(255);

    cv::Mat inverted_mask;
    cv::bitwise_not(single_channel, inverted_mask);
    cv::bitwise_and(inverted_mask, band_mask, inverted_mask);

    const cv::Mat morph_kernel =
        cv::getStructuringElement(cv::MORPH_RECT, config_.morph_kernel);
    cv::Mat cleaned_sides;
    cv::morphologyEx(inverted_mask, cleaned_sides, cv::MORPH_CLOSE, morph_kernel,
                     {-1, -1}, config_.morph_iterations);

    std::vector<std::vector<cv::Point>> side_contours;
    cv::findContours(cleaned_sides, side_contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    auto is_large_enough = [&](const std::vector<cv::Point> &contour) {
        return cv::contourArea(contour) >= config_.min_contour_area;
    };

    std::vector<std::vector<cv::Point>> filtered_sides;
    std::copy_if(side_contours.begin(), side_contours.end(),
                 std::back_inserter(filtered_sides), is_large_enough);

    std::sort(filtered_sides.begin(), filtered_sides.end(), [](const auto &lhs, const auto &rhs) {
        return cv::contourArea(lhs) > cv::contourArea(rhs);
    });

    const auto compute_centroid = [](const std::vector<cv::Point> &contour) {
        cv::Moments m = cv::moments(contour);
        if (m.m00 == 0.0) {
            return cv::Point2f(0.0F, 0.0F);
        }
        return cv::Point2f(static_cast<float>(m.m10 / m.m00),
                           static_cast<float>(m.m01 / m.m00));
    };

    std::vector<cv::Point> left_side;
    std::vector<cv::Point> right_side;
    const int center_x = result.frame_center.x;
    if (!filtered_sides.empty()) {
        const auto &first = filtered_sides[0];
        const bool has_second = filtered_sides.size() > 1;
        const auto &second = has_second ? filtered_sides[1] : filtered_sides[0];

        cv::Point2f first_centroid = compute_centroid(first);
        cv::Point2f second_centroid = compute_centroid(second);

        if (has_second && first_centroid.x > second_centroid.x) {
            left_side = second;
            right_side = first;
        } else {
            left_side = first;
            right_side = second;
        }

        if (!has_second) {
            if (first_centroid.x < center_x) {
                left_side = first;
                right_side.clear();
            } else {
                right_side = first;
                left_side.clear();
            }
        }
    }

    cv::Mat left_mask = cv::Mat::zeros(single_channel.size(), single_channel.type());
    cv::Mat right_mask = cv::Mat::zeros(single_channel.size(), single_channel.type());
    if (!left_side.empty()) {
        cv::drawContours(left_mask, {left_side}, -1, cv::Scalar(255), cv::FILLED);
        result.left_boundary = FitBoundarySegment(left_side, single_channel.cols);
    }
    if (!right_side.empty()) {
        cv::drawContours(right_mask, {right_side}, -1, cv::Scalar(255), cv::FILLED);
        result.right_boundary = FitBoundarySegment(right_side, single_channel.cols);
    }

    cv::Mat road_mask;
    cv::bitwise_not(left_mask | right_mask, road_mask);
    if (road_mask.empty()) {
        road_mask = single_channel.clone();
    }

    cv::bitwise_and(road_mask, band_mask, road_mask);

    cv::morphologyEx(road_mask, road_mask, cv::MORPH_CLOSE, morph_kernel, {-1, -1},
                     config_.morph_iterations);
    result.left_mask = left_mask;
    result.right_mask = right_mask;
    result.road_mask = road_mask;
    result.processed_frame = road_mask;

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(road_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
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

    const auto samples = CollectBoundarySamples(road_mask, bounds, result.lane_center.x);
    if (!result.left_boundary.valid) {
        result.left_boundary = FitBoundarySegment(samples.left, road_mask.cols);
    }
    if (!result.left_boundary.valid && fallback_left_x < road_mask.cols) {
        result.left_boundary = BuildVerticalFallback(fallback_left_x, band_start, bottom_row,
                                                     road_mask.cols);
    }

    if (!result.right_boundary.valid) {
        result.right_boundary = FitBoundarySegment(samples.right, road_mask.cols);
    }
    if (!result.right_boundary.valid && fallback_right_x >= 0 &&
        fallback_right_x < road_mask.cols) {
        result.right_boundary = BuildVerticalFallback(fallback_right_x, band_start, bottom_row,
                                                      road_mask.cols);
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
