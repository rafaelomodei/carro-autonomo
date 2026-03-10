#include "pipeline/BoundaryAnalyzer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline {
namespace {

struct RowExtent {
    int y{0};
    int left{-1};
    int right{-1};
};

struct CandidateComponent {
    cv::Mat mask;
    std::vector<RowExtent> extents;
    double area{0.0};
    cv::Rect bounds;
    double score{0.0};
    double center_alignment{0.0};
};

std::vector<RowExtent> collectRowExtents(const cv::Mat &mask, int step) {
    std::vector<RowExtent> extents;
    if (mask.empty()) {
        return extents;
    }

    step = std::max(1, step);
    for (int y = 0; y < mask.rows; y += step) {
        const auto *row = mask.ptr<uchar>(y);
        int left = -1;
        int right = -1;
        for (int x = 0; x < mask.cols; ++x) {
            if (row[x] > 0) {
                left = x;
                break;
            }
        }
        for (int x = mask.cols - 1; x >= 0; --x) {
            if (row[x] > 0) {
                right = x;
                break;
            }
        }

        if (left >= 0 && right >= left) {
            extents.push_back({y, left, right});
        }
    }

    return extents;
}

double average(const std::vector<double> &values, double fallback = 0.0) {
    if (values.empty()) {
        return fallback;
    }

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / static_cast<double>(values.size());
}

std::vector<cv::Point> smoothPoints(const std::vector<cv::Point> &points, int window_size) {
    if (points.size() < 3 || window_size <= 1) {
        return points;
    }

    std::vector<cv::Point> smoothed;
    smoothed.reserve(points.size());
    const int half_window = window_size / 2;

    for (int index = 0; index < static_cast<int>(points.size()); ++index) {
        const int begin = std::max(0, index - half_window);
        const int end = std::min(static_cast<int>(points.size()) - 1, index + half_window);

        double sum_x = 0.0;
        double sum_y = 0.0;
        int count = 0;
        for (int cursor = begin; cursor <= end; ++cursor) {
            sum_x += static_cast<double>(points[cursor].x);
            sum_y += static_cast<double>(points[cursor].y);
            ++count;
        }

        smoothed.emplace_back(static_cast<int>(std::lround(sum_x / static_cast<double>(count))),
                              static_cast<int>(std::lround(sum_y / static_cast<double>(count))));
    }

    return smoothed;
}

double clamp01(double value) { return std::clamp(value, 0.0, 1.0); }

double computeCenterAlignment(const std::vector<RowExtent> &extents, int image_width) {
    if (extents.empty() || image_width <= 1) {
        return 0.0;
    }

    const int bottom_band_start = static_cast<int>(std::floor(extents.back().y * 0.75));
    std::vector<double> bottom_midpoints;
    for (const RowExtent &extent : extents) {
        if (extent.y >= bottom_band_start) {
            bottom_midpoints.push_back((static_cast<double>(extent.left) +
                                        static_cast<double>(extent.right)) /
                                       2.0);
        }
    }

    const double center_x = (static_cast<double>(image_width) - 1.0) / 2.0;
    const double bottom_midpoint = average(bottom_midpoints, center_x);
    const double delta = std::abs(bottom_midpoint - center_x);
    return 1.0 - clamp01(delta / std::max(1.0, center_x));
}

std::optional<CandidateComponent> selectBestComponent(const cv::Mat &binary_mask, double min_area) {
    if (binary_mask.empty()) {
        return std::nullopt;
    }

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int component_count =
        cv::connectedComponentsWithStats(binary_mask, labels, stats, centroids, 8, CV_32S);
    if (component_count <= 1) {
        return std::nullopt;
    }

    const double roi_area = static_cast<double>(binary_mask.rows * binary_mask.cols);
    const int scanline_step = 4;

    std::optional<CandidateComponent> best;
    for (int label = 1; label < component_count; ++label) {
        const double area = static_cast<double>(stats.at<int>(label, cv::CC_STAT_AREA));
        if (area < min_area) {
            continue;
        }

        cv::Mat component_mask;
        cv::compare(labels, label, component_mask, cv::CMP_EQ);
        const std::vector<RowExtent> extents = collectRowExtents(component_mask, scanline_step);
        if (extents.size() < 4) {
            continue;
        }

        const cv::Rect bounds(stats.at<int>(label, cv::CC_STAT_LEFT),
                              stats.at<int>(label, cv::CC_STAT_TOP),
                              stats.at<int>(label, cv::CC_STAT_WIDTH),
                              stats.at<int>(label, cv::CC_STAT_HEIGHT));
        const double area_score = clamp01(area / std::max(1.0, roi_area * 0.35));
        const double vertical_span = clamp01(
            static_cast<double>(bounds.height) / std::max(1.0, static_cast<double>(binary_mask.rows)));
        const double scanline_coverage = clamp01(
            static_cast<double>(extents.size()) /
            std::max(1.0, static_cast<double>(binary_mask.rows) / static_cast<double>(scanline_step)));
        const double center_alignment = computeCenterAlignment(extents, binary_mask.cols);
        const double score =
            (0.40 * area_score) + (0.25 * vertical_span) + (0.20 * scanline_coverage) +
            (0.15 * center_alignment);

        if (!best || score > best->score) {
            best = CandidateComponent{
                component_mask, extents, area, bounds, score, center_alignment,
            };
        }
    }

    return best;
}

LaneBoundarySegment summarizeBoundary(const std::vector<cv::Point> &points) {
    LaneBoundarySegment boundary;
    if (points.size() < 2) {
        return boundary;
    }

    boundary.top = points.front();
    boundary.bottom = points.back();
    boundary.valid = true;
    return boundary;
}

} // namespace

BoundaryAnalyzer::BoundaryAnalyzer(double min_contour_area) : min_contour_area_(min_contour_area) {}

RoadSegmentationResult BoundaryAnalyzer::analyze(const cv::Mat &mask, const cv::Size &frame_size,
                                                 const cv::Rect &roi_rect,
                                                 const std::string &segmentation_mode) const {
    RoadSegmentationResult result;
    result.segmentation_mode = segmentation_mode;
    result.roi_rect = roi_rect;

    const double frame_center_x =
        frame_size.width > 0 ? (static_cast<double>(frame_size.width) - 1.0) / 2.0 : 0.0;
    result.frame_center =
        cv::Point(static_cast<int>(std::lround(frame_center_x)), std::max(0, frame_size.height - 1));
    result.lane_center = result.frame_center;

    if (mask.empty() || roi_rect.width <= 0 || roi_rect.height <= 0) {
        return result;
    }

    cv::Mat single_channel;
    if (mask.channels() == 1) {
        single_channel = mask;
    } else {
        cv::cvtColor(mask, single_channel, cv::COLOR_BGR2GRAY);
    }

    const auto candidate = selectBestComponent(single_channel, min_contour_area_);
    if (!candidate) {
        return result;
    }
    result.mask_area_px = candidate->area;
    result.confidence_score = clamp01(candidate->score);
    result.mask_frame = candidate->mask.clone();

    const cv::Point origin(roi_rect.x, roi_rect.y);
    std::vector<cv::Point> left_local;
    std::vector<cv::Point> right_local;
    std::vector<cv::Point> center_local;
    std::vector<double> widths;
    left_local.reserve(candidate->extents.size());
    right_local.reserve(candidate->extents.size());
    center_local.reserve(candidate->extents.size());

    for (const RowExtent &extent : candidate->extents) {
        if (extent.right <= extent.left) {
            continue;
        }

        left_local.emplace_back(extent.left, extent.y);
        right_local.emplace_back(extent.right, extent.y);
        center_local.emplace_back(static_cast<int>(std::lround(
                                    (static_cast<double>(extent.left) +
                                     static_cast<double>(extent.right)) /
                                    2.0)),
                                  extent.y);
        widths.push_back(static_cast<double>(extent.right - extent.left));
    }

    left_local = smoothPoints(left_local, 5);
    right_local = smoothPoints(right_local, 5);
    center_local = smoothPoints(center_local, 5);

    result.left_boundary_points.reserve(left_local.size());
    result.right_boundary_points.reserve(right_local.size());
    result.centerline_points.reserve(center_local.size());
    for (const cv::Point &point : left_local) {
        result.left_boundary_points.emplace_back(point + origin);
    }
    for (const cv::Point &point : right_local) {
        result.right_boundary_points.emplace_back(point + origin);
    }
    for (const cv::Point &point : center_local) {
        result.centerline_points.emplace_back(point + origin);
    }

    result.left_boundary = summarizeBoundary(result.left_boundary_points);
    result.right_boundary = summarizeBoundary(result.right_boundary_points);

    result.road_polygon_points = result.left_boundary_points;
    for (auto it = result.right_boundary_points.rbegin(); it != result.right_boundary_points.rend(); ++it) {
        result.road_polygon_points.push_back(*it);
    }

    const std::size_t tail_count = std::min<std::size_t>(5, result.centerline_points.size());
    if (tail_count > 0) {
        double lane_center_x = 0.0;
        double lane_center_y = 0.0;
        for (std::size_t index = result.centerline_points.size() - tail_count;
             index < result.centerline_points.size(); ++index) {
            lane_center_x += static_cast<double>(result.centerline_points[index].x);
            lane_center_y += static_cast<double>(result.centerline_points[index].y);
        }
        result.lane_center = cv::Point(
            static_cast<int>(std::lround(lane_center_x / static_cast<double>(tail_count))),
            static_cast<int>(std::lround(lane_center_y / static_cast<double>(tail_count))));
    }

    result.lane_width_px = average(widths, static_cast<double>(candidate->bounds.width));
    result.lateral_offset_px = static_cast<double>(result.lane_center.x) - frame_center_x;
    if (frame_size.width > 1) {
        result.lane_center_ratio = std::clamp(
            static_cast<double>(result.lane_center.x) / static_cast<double>(frame_size.width - 1), 0.0,
            1.0);
    }
    if (frame_center_x > 0.0) {
        result.steering_error_normalized =
            std::clamp(result.lateral_offset_px / frame_center_x, -1.0, 1.0);
    }

    result.lane_found = result.left_boundary.valid && result.right_boundary.valid &&
                        result.road_polygon_points.size() >= 6 && result.confidence_score >= 0.25;
    return result;
}

} // namespace road_segmentation_lab::pipeline
