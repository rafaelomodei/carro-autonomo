#include "services/camera/LaneDetector.hpp"

#include <algorithm>
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

    int left_x = single_channel.cols;
    int right_x = 0;
    for (const auto &point : largest_contour) {
        if (point.y >= band_start) {
            left_x = std::min(left_x, point.x);
            right_x = std::max(right_x, point.x);
        }
    }

    if (left_x > right_x) {
        const cv::Rect bounds = cv::boundingRect(largest_contour);
        left_x = bounds.x;
        right_x = bounds.x + bounds.width;
    }

    result.left_boundary = cv::Point(left_x, bottom_row);
    result.right_boundary = cv::Point(right_x, bottom_row);

    const cv::Moments contour_moments = cv::moments(largest_contour);
    if (contour_moments.m00 != 0.0) {
        result.lane_center = cv::Point(static_cast<int>(contour_moments.m10 / contour_moments.m00),
                                       static_cast<int>(contour_moments.m01 / contour_moments.m00));
    } else {
        result.lane_center = cv::Point((left_x + right_x) / 2, bottom_row - band_height / 2);
    }

    result.lane_found = true;
    result.lateral_offset_px = static_cast<double>(result.lane_center.x - result.frame_center.x);
    if (frame_size.width > 0) {
        result.lateral_offset_percentage =
            (result.lateral_offset_px / (static_cast<double>(frame_size.width) / 2.0)) * 100.0;
    }

    return result;
}

} // namespace autonomous_car::services::camera
