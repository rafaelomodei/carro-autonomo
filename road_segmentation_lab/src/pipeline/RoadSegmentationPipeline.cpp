#include "pipeline/RoadSegmentationPipeline.hpp"

#include <algorithm>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline {
namespace {

std::vector<cv::Point> buildRoiPolygon(const config::LabConfig &config, const cv::Size &size) {
    if (size.width <= 0 || size.height <= 0) {
        return {};
    }

    const double center_x =
        std::clamp(config.roi_polygon_center_x_ratio, 0.0, 1.0) * static_cast<double>(size.width - 1);
    const int top_half_width = static_cast<int>(
        std::lround((config.roi_polygon_top_width_ratio * static_cast<double>(size.width)) / 2.0));
    const int bottom_half_width = static_cast<int>(
        std::lround((config.roi_polygon_bottom_width_ratio * static_cast<double>(size.width)) / 2.0));
    const auto clamp_x = [&](int value) { return std::clamp(value, 0, size.width - 1); };

    return {
        {clamp_x(static_cast<int>(std::lround(center_x)) - top_half_width), 0},
        {clamp_x(static_cast<int>(std::lround(center_x)) + top_half_width), 0},
        {clamp_x(static_cast<int>(std::lround(center_x)) + bottom_half_width), size.height - 1},
        {clamp_x(static_cast<int>(std::lround(center_x)) - bottom_half_width), size.height - 1},
    };
}

std::vector<cv::Point> buildHoodMaskPolygon(const config::LabConfig &config, const cv::Size &size) {
    if (!config.hood_mask_enabled || size.width <= 0 || size.height <= 0) {
        return {};
    }

    const cv::Point center(
        static_cast<int>(std::lround(config.hood_mask_center_x_ratio * static_cast<double>(size.width - 1))),
        static_cast<int>(std::lround(
            static_cast<double>(size.height - 1) -
            (config.hood_mask_bottom_offset_ratio * static_cast<double>(size.height)) -
            ((config.hood_mask_height_ratio * static_cast<double>(size.height)) / 2.0))));
    const cv::Size axes(
        std::max(1, static_cast<int>(
                        std::lround((config.hood_mask_width_ratio * static_cast<double>(size.width)) / 2.0))),
        std::max(1, static_cast<int>(
                        std::lround((config.hood_mask_height_ratio * static_cast<double>(size.height)) / 2.0))));

    std::vector<cv::Point> polygon;
    cv::ellipse2Poly(center, axes, 0, 0, 360, 10, polygon);
    return polygon;
}

cv::Mat buildSpatialMask(const cv::Size &size, const std::vector<cv::Point> &roi_polygon,
                         const std::vector<cv::Point> &hood_polygon) {
    cv::Mat spatial_mask(size, CV_8UC1, cv::Scalar(0));
    if (!roi_polygon.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {roi_polygon};
        cv::fillPoly(spatial_mask, polygons, cv::Scalar(255));
    } else {
        spatial_mask.setTo(cv::Scalar(255));
    }

    if (!hood_polygon.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {hood_polygon};
        cv::fillPoly(spatial_mask, polygons, cv::Scalar(0));
    }

    return spatial_mask;
}

cv::Mat maskColorFrame(const cv::Mat &frame, const cv::Mat &mask) {
    if (frame.empty()) {
        return cv::Mat();
    }

    cv::Mat masked(frame.size(), frame.type(), cv::Scalar(0, 0, 0));
    frame.copyTo(masked, mask);
    return masked;
}

std::vector<cv::Point> translatePoints(const std::vector<cv::Point> &points, const cv::Point &offset) {
    std::vector<cv::Point> translated;
    translated.reserve(points.size());
    for (const cv::Point &point : points) {
        translated.emplace_back(point + offset);
    }
    return translated;
}

} // namespace

RoadSegmentationPipeline::RoadSegmentationPipeline() { updateConfig(config::LabConfig{}); }

RoadSegmentationPipeline::RoadSegmentationPipeline(const config::LabConfig &config) {
    updateConfig(config);
}

void RoadSegmentationPipeline::updateConfig(const config::LabConfig &config) {
    config_ = config;
    resize_stage_ =
        stages::ResizeStage(config_.resize_enabled, {config_.target_width, config_.target_height});
    undistort_stage_ = stages::UndistortStage(config_.calibration_file);
    roi_stage_ = stages::RoiStage(config_.roi_top_ratio, config_.roi_bottom_ratio);
    gaussian_stage_ =
        stages::GaussianStage(config_.gaussian_enabled, config_.gaussian_kernel, config_.gaussian_sigma);
    illumination_stage_ = stages::IlluminationStage(config_.clahe_enabled);
    segmentation_stage_ = stages::SegmentationStage(config_);
    morphology_stage_ =
        stages::MorphologyStage(config_.morph_enabled, config_.morph_kernel, config_.morph_iterations);
    boundary_analyzer_ = BoundaryAnalyzer(config_.min_contour_area);
}

const config::LabConfig &RoadSegmentationPipeline::config() const noexcept { return config_; }

std::string RoadSegmentationPipeline::calibrationStatus() const {
    return undistort_stage_.statusMessage();
}

RoadSegmentationResult RoadSegmentationPipeline::process(const cv::Mat &frame) const {
    RoadSegmentationResult result;
    if (frame.empty()) {
        return result;
    }

    cv::Mat resized = resize_stage_.apply(frame);
    cv::Mat corrected = undistort_stage_.apply(resized);

    cv::Rect roi_rect;
    cv::Mat roi = roi_stage_.apply(corrected, &roi_rect);
    cv::Mat blurred = gaussian_stage_.apply(roi);
    cv::Mat illuminated = illumination_stage_.apply(blurred);
    const std::vector<cv::Point> roi_polygon = buildRoiPolygon(config_, roi.size());
    const std::vector<cv::Point> hood_polygon = buildHoodMaskPolygon(config_, roi.size());
    const cv::Mat spatial_mask = buildSpatialMask(roi.size(), roi_polygon, hood_polygon);
    cv::Mat mask = segmentation_stage_.apply(illuminated);
    cv::Mat masked_binary;
    cv::bitwise_and(mask, spatial_mask, masked_binary);
    cv::Mat filtered_mask = morphology_stage_.apply(masked_binary);
    cv::bitwise_and(filtered_mask, spatial_mask, filtered_mask);

    result = boundary_analyzer_.analyze(filtered_mask, corrected.size(), roi_rect,
                                        segmentation_stage_.modeName());
    result.resized_frame = corrected;
    result.roi_frame = roi;
    result.preprocessed_frame = maskColorFrame(illuminated, spatial_mask);
    if (result.mask_frame.empty()) {
        result.mask_frame = filtered_mask;
    }
    result.roi_polygon_points = translatePoints(roi_polygon, roi_rect.tl());
    result.hood_mask_polygon_points = translatePoints(hood_polygon, roi_rect.tl());
    return result;
}

} // namespace road_segmentation_lab::pipeline
