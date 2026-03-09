#include "render/DebugRenderer.hpp"

#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::render {
namespace {

const cv::Scalar kRoiColor{0, 215, 255};
const cv::Scalar kLeftBoundaryColor{255, 0, 0};
const cv::Scalar kRightBoundaryColor{0, 215, 255};
const cv::Scalar kLaneCenterColor{0, 255, 0};
const cv::Scalar kFrameCenterColor{255, 255, 255};
const cv::Scalar kBackground{18, 18, 18};
const cv::Scalar kHoodMaskColor{255, 0, 255};
const cv::Scalar kLaneFillColor{40, 170, 90};

std::string formatDouble(double value, int precision = 3) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

} // namespace

cv::Mat DebugRenderer::render(const pipeline::RoadSegmentationResult &result,
                              const config::LabConfig &config, const std::string &source_label,
                              const std::string &calibration_status) const {
    if (result.resized_frame.empty()) {
        return cv::Mat();
    }

    const cv::Mat original_tile = buildOriginalTile(result, source_label);
    const cv::Mat preprocess_tile = buildPreprocessTile(result, config, calibration_status);
    const cv::Mat mask_tile = buildMaskTile(result);
    const cv::Mat output_tile = buildOutputTile(result);

    cv::Mat top_row;
    cv::Mat bottom_row;
    cv::hconcat(original_tile, preprocess_tile, top_row);
    cv::hconcat(mask_tile, output_tile, bottom_row);

    cv::Mat panel;
    cv::vconcat(top_row, bottom_row, panel);
    return panel;
}

cv::Mat DebugRenderer::buildOriginalTile(const pipeline::RoadSegmentationResult &result,
                                         const std::string &source_label) const {
    cv::Mat tile = result.resized_frame.clone();
    if (!result.roi_polygon_points.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {result.roi_polygon_points};
        cv::polylines(tile, polygons, true, kRoiColor, 2, cv::LINE_AA);
    } else if (result.roi_rect.area() > 0) {
        cv::rectangle(tile, result.roi_rect, kRoiColor, 2, cv::LINE_AA);
    }
    if (!result.hood_mask_polygon_points.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {result.hood_mask_polygon_points};
        cv::polylines(tile, polygons, true, kHoodMaskColor, 2, cv::LINE_AA);
    }

    drawLabelBlock(tile, "Original",
                   {source_label,
                    "ROI: trapezio util",
                    "Capo: " +
                        std::string(result.hood_mask_polygon_points.empty() ? "off" : "masked")});
    return tile;
}

cv::Mat DebugRenderer::buildPreprocessTile(const pipeline::RoadSegmentationResult &result,
                                           const config::LabConfig &config,
                                           const std::string &calibration_status) const {
    cv::Mat tile(result.resized_frame.size(), CV_8UC3, kBackground);

    if (!result.preprocessed_frame.empty() && result.roi_rect.area() > 0) {
        cv::Mat preprocessed = ensureColor(result.preprocessed_frame);
        preprocessed.copyTo(tile(result.roi_rect));
    }

    if (!result.roi_polygon_points.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {result.roi_polygon_points};
        cv::polylines(tile, polygons, true, kRoiColor, 1, cv::LINE_AA);
    }
    if (!result.hood_mask_polygon_points.empty()) {
        std::vector<std::vector<cv::Point>> polygons = {result.hood_mask_polygon_points};
        cv::polylines(tile, polygons, true, kHoodMaskColor, 1, cv::LINE_AA);
    }

    drawLabelBlock(tile, "ROI + preprocessamento",
                   {"Seg: " + result.segmentation_mode,
                    "Blur: " + std::string(config.gaussian_enabled ? "on" : "off"),
                    "CLAHE: " + std::string(config.clahe_enabled ? "on" : "off"),
                    "Conf.: " + formatDouble(result.confidence_score, 2),
                    calibration_status});
    return tile;
}

cv::Mat DebugRenderer::buildMaskTile(const pipeline::RoadSegmentationResult &result) const {
    cv::Mat tile(result.resized_frame.size(), CV_8UC3, kBackground);
    if (!result.mask_frame.empty() && result.roi_rect.area() > 0) {
        cv::Mat mask_color;
        cv::applyColorMap(result.mask_frame, mask_color, cv::COLORMAP_BONE);
        mask_color.copyTo(tile(result.roi_rect));
    }

    drawLabelBlock(tile, "Mascara binaria",
                   {"Area: " + formatDouble(result.mask_area_px, 0) + " px2",
                    "Largura: " + formatDouble(result.lane_width_px, 1) + " px",
                    "Score: " + formatDouble(result.confidence_score, 2)});
    return tile;
}

cv::Mat DebugRenderer::buildOutputTile(const pipeline::RoadSegmentationResult &result) const {
    cv::Mat tile = result.resized_frame.clone();

    if (!result.road_polygon_points.empty()) {
        cv::Mat overlay = tile.clone();
        std::vector<std::vector<cv::Point>> polygons = {result.road_polygon_points};
        cv::fillPoly(overlay, polygons, kLaneFillColor);
        cv::addWeighted(overlay, 0.28, tile, 0.72, 0.0, tile);
    }

    cv::line(tile, {result.frame_center.x, 0}, {result.frame_center.x, tile.rows},
             kFrameCenterColor, 1, cv::LINE_AA);

    if (result.left_boundary_points.size() >= 2) {
        std::vector<std::vector<cv::Point>> polylines = {result.left_boundary_points};
        cv::polylines(tile, polylines, false, kLeftBoundaryColor, 2, cv::LINE_AA);
    }
    if (result.right_boundary_points.size() >= 2) {
        std::vector<std::vector<cv::Point>> polylines = {result.right_boundary_points};
        cv::polylines(tile, polylines, false, kRightBoundaryColor, 2, cv::LINE_AA);
    }
    if (result.centerline_points.size() >= 2) {
        std::vector<std::vector<cv::Point>> polylines = {result.centerline_points};
        cv::polylines(tile, polylines, false, kLaneCenterColor, 2, cv::LINE_AA);
    }

    if (result.lane_found) {
        cv::circle(tile, result.lane_center, 6, kLaneCenterColor, cv::FILLED);
        cv::line(tile, result.frame_center, result.lane_center, kLaneCenterColor, 2, cv::LINE_AA);
    } else {
        cv::putText(tile, "Pista nao detectada", {20, tile.rows - 20}, cv::FONT_HERSHEY_SIMPLEX,
                    0.8, {0, 0, 255}, 2, cv::LINE_AA);
    }

    drawLabelBlock(tile, "Saida anotada",
                   {"Lane ratio: " + formatDouble(result.lane_center_ratio),
                    "Erro norm.: " + formatDouble(result.steering_error_normalized),
                    "Offset: " + formatDouble(result.lateral_offset_px, 1) + " px",
                    "Confianca: " + formatDouble(result.confidence_score, 2),
                    "Status: " + std::string(result.lane_found ? "detected" : "missing")});
    return tile;
}

cv::Mat DebugRenderer::ensureColor(const cv::Mat &frame) {
    if (frame.empty()) {
        return cv::Mat();
    }
    if (frame.channels() == 3) {
        return frame.clone();
    }
    cv::Mat color;
    cv::cvtColor(frame, color, cv::COLOR_GRAY2BGR);
    return color;
}

void DebugRenderer::drawLabelBlock(cv::Mat &image, const std::string &title,
                                   const std::vector<std::string> &lines) {
    const int block_height = 28 + static_cast<int>(lines.size()) * 20;
    cv::rectangle(image, {8, 8}, {image.cols - 8, 8 + block_height}, {0, 0, 0}, cv::FILLED);
    cv::putText(image, title, {16, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {255, 255, 255}, 2,
                cv::LINE_AA);

    int y = 52;
    for (const std::string &line : lines) {
        cv::putText(image, line, {16, y}, cv::FONT_HERSHEY_SIMPLEX, 0.5, {220, 220, 220}, 1,
                    cv::LINE_AA);
        y += 20;
    }
}

} // namespace road_segmentation_lab::render
