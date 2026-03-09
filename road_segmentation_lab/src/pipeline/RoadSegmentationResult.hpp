#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace road_segmentation_lab::pipeline {

struct LaneBoundarySegment {
    cv::Point top;
    cv::Point bottom;
    bool valid{false};
};

struct RoadSegmentationResult {
    bool lane_found{false};
    LaneBoundarySegment left_boundary;
    LaneBoundarySegment right_boundary;
    cv::Point lane_center;
    cv::Point frame_center;
    cv::Rect roi_rect;
    double lane_center_ratio{0.5};
    double steering_error_normalized{0.0};
    double lateral_offset_px{0.0};
    double lane_width_px{0.0};
    double mask_area_px{0.0};
    double confidence_score{0.0};
    std::string segmentation_mode;
    std::vector<cv::Point> left_boundary_points;
    std::vector<cv::Point> right_boundary_points;
    std::vector<cv::Point> centerline_points;
    std::vector<cv::Point> road_polygon_points;
    std::vector<cv::Point> roi_polygon_points;
    std::vector<cv::Point> hood_mask_polygon_points;
    cv::Mat resized_frame;
    cv::Mat roi_frame;
    cv::Mat preprocessed_frame;
    cv::Mat mask_frame;
};

} // namespace road_segmentation_lab::pipeline
