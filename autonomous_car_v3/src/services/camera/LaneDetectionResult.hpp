#pragma once

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera {

struct LaneDetectionResult {
    bool lane_found{false};
    cv::Point left_boundary;
    cv::Point right_boundary;
    cv::Point lane_center;
    cv::Point frame_center;
    double lateral_offset_px{0.0};
    double lateral_offset_percentage{0.0};
    cv::Mat processed_frame;
};

} // namespace autonomous_car::services::camera
