#pragma once

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera {

struct LaneBoundarySegment {
    cv::Point top;
    cv::Point bottom;
    bool valid{false};
};

struct LaneDetectionResult {
    bool lane_found{false};
    LaneBoundarySegment left_boundary;
    LaneBoundarySegment right_boundary;
    cv::Point lane_center;
    cv::Point horizon_point;
    cv::Point frame_center;
    double lateral_offset_px{0.0};
    double lateral_offset_percentage{0.0};
    cv::Mat processed_frame;
    bool horizon_found{false};
};

} // namespace autonomous_car::services::camera
