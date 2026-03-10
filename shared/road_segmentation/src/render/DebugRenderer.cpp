#include "render/DebugRenderer.hpp"

#include <algorithm>
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
const cv::Scalar kFarReferenceColor{90, 90, 255};
const cv::Scalar kMidReferenceColor{255, 180, 0};
const cv::Scalar kNearReferenceColor{60, 220, 120};
const cv::Scalar kSidebarBackground{26, 26, 26};
const cv::Scalar kSidebarBorder{64, 64, 64};
constexpr int kSidebarWidth = 250;

std::string formatDouble(double value, int precision = 3) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string formatReferenceStatus(const pipeline::LookaheadReference &reference) {
    return reference.valid ? "ok" : "missing";
}

cv::Scalar referenceColor(const std::string &label) {
    if (label == "FAR") {
        return kFarReferenceColor;
    }
    if (label == "MID") {
        return kMidReferenceColor;
    }
    return kNearReferenceColor;
}

void drawReferenceBand(cv::Mat &image, const pipeline::LookaheadReference &reference,
                       const cv::Rect &roi_rect, const std::string &label) {
    if (image.empty() || roi_rect.area() <= 0 || reference.bottom_y < reference.top_y) {
        return;
    }

    const int top_y = std::clamp(reference.top_y, 0, image.rows - 1);
    const int bottom_y = std::clamp(reference.bottom_y, 0, image.rows - 1);
    if (bottom_y < top_y) {
        return;
    }

    const cv::Scalar color = referenceColor(label);
    cv::Mat overlay = image.clone();
    cv::rectangle(overlay, {roi_rect.x, top_y}, {roi_rect.x + roi_rect.width - 1, bottom_y}, color,
                  cv::FILLED);
    cv::addWeighted(overlay, 0.14, image, 0.86, 0.0, image);
    cv::rectangle(image, {roi_rect.x, top_y}, {roi_rect.x + roi_rect.width - 1, bottom_y}, color, 1,
                  cv::LINE_AA);

    const int text_y = std::clamp(top_y + 18, 16, std::max(16, bottom_y - 4));
    cv::putText(image, label, {roi_rect.x + 8, text_y}, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1,
                cv::LINE_AA);
}

void drawReferencePoint(cv::Mat &image, const pipeline::LookaheadReference &reference,
                        const std::string &label) {
    if (image.empty() || !reference.valid) {
        return;
    }

    const cv::Scalar color = referenceColor(label);
    cv::circle(image, reference.point, 5, color, cv::FILLED, cv::LINE_AA);
    cv::putText(image, label, reference.point + cv::Point(8, -8), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                color, 1, cv::LINE_AA);
}

void drawLookaheadOverlay(cv::Mat &image, const pipeline::RoadSegmentationResult &result) {
    if (image.empty() || result.roi_rect.area() <= 0) {
        return;
    }

    drawReferenceBand(image, result.far_reference, result.roi_rect, "FAR");
    drawReferenceBand(image, result.mid_reference, result.roi_rect, "MID");
    drawReferenceBand(image, result.near_reference, result.roi_rect, "NEAR");

    drawReferencePoint(image, result.far_reference, "FAR");
    drawReferencePoint(image, result.mid_reference, "MID");
    drawReferencePoint(image, result.near_reference, "NEAR");
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
    drawLookaheadOverlay(tile, result);
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

    return composeTileCard(tile, "Original",
                           {source_label,
                            "ROI: trapezio util",
                            "Lookahead: FAR/MID/NEAR",
                            "Capo: " +
                                std::string(result.hood_mask_polygon_points.empty() ? "off" : "masked")});
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

    return composeTileCard(tile, "ROI + preprocessamento",
                           {"Seg: " + result.segmentation_mode,
                            "Blur: " + std::string(config.gaussian_enabled ? "on" : "off"),
                            "CLAHE: " + std::string(config.clahe_enabled ? "on" : "off"),
                            "Conf.: " + formatDouble(result.confidence_score, 2),
                            calibration_status});
}

cv::Mat DebugRenderer::buildMaskTile(const pipeline::RoadSegmentationResult &result) const {
    cv::Mat tile(result.resized_frame.size(), CV_8UC3, kBackground);
    if (!result.mask_frame.empty() && result.roi_rect.area() > 0) {
        cv::Mat mask_color;
        cv::applyColorMap(result.mask_frame, mask_color, cv::COLORMAP_BONE);
        mask_color.copyTo(tile(result.roi_rect));
    }

    return composeTileCard(tile, "Mascara binaria",
                           {"Area: " + formatDouble(result.mask_area_px, 0) + " px2",
                            "Largura: " + formatDouble(result.lane_width_px, 1) + " px",
                            "Score: " + formatDouble(result.confidence_score, 2)});
}

cv::Mat DebugRenderer::buildOutputTile(const pipeline::RoadSegmentationResult &result) const {
    cv::Mat tile = result.resized_frame.clone();

    if (!result.road_polygon_points.empty()) {
        cv::Mat overlay = tile.clone();
        std::vector<std::vector<cv::Point>> polygons = {result.road_polygon_points};
        cv::fillPoly(overlay, polygons, kLaneFillColor);
        cv::addWeighted(overlay, 0.28, tile, 0.72, 0.0, tile);
    }

    drawLookaheadOverlay(tile, result);

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
        cv::rectangle(tile, {2, 2}, {tile.cols - 3, tile.rows - 3}, {0, 0, 255}, 2, cv::LINE_AA);
    }

    return composeTileCard(tile, "Saida anotada",
                           {"Lane ratio: " + formatDouble(result.lane_center_ratio),
                            "Erro norm.: " + formatDouble(result.steering_error_normalized),
                            "Offset: " + formatDouble(result.lateral_offset_px, 1) + " px",
                            "Near: " + formatReferenceStatus(result.near_reference) + " off=" +
                                formatDouble(result.near_reference.lateral_offset_px, 1),
                            "Mid: " + formatReferenceStatus(result.mid_reference) + " off=" +
                                formatDouble(result.mid_reference.lateral_offset_px, 1),
                            "Far: " + formatReferenceStatus(result.far_reference) + " off=" +
                                formatDouble(result.far_reference.lateral_offset_px, 1),
                            "Heading: " + std::string(result.heading_valid
                                                          ? formatDouble(result.heading_error_rad)
                                                          : "missing"),
                            "Curv.: " + std::string(result.curvature_valid
                                                        ? formatDouble(result.curvature_indicator_rad)
                                                        : "missing"),
                            "Confianca: " + formatDouble(result.confidence_score, 2),
                            "Status: " + std::string(result.lane_found ? "detected" : "missing")});
}

cv::Mat DebugRenderer::composeTileCard(const cv::Mat &image, const std::string &title,
                                       const std::vector<std::string> &lines) {
    cv::Mat content = ensureColor(image);
    if (content.empty()) {
        return cv::Mat();
    }

    cv::Mat sidebar(content.rows, kSidebarWidth, CV_8UC3, kSidebarBackground);
    drawLabelBlock(sidebar, title, lines);
    cv::line(sidebar, {0, 0}, {0, sidebar.rows - 1}, kSidebarBorder, 1, cv::LINE_AA);

    cv::Mat card;
    cv::hconcat(content, sidebar, card);
    return card;
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
    if (image.empty()) {
        return;
    }

    cv::rectangle(image, {0, 0}, {image.cols - 1, image.rows - 1}, kSidebarBorder, 1, cv::LINE_AA);
    cv::rectangle(image, {10, 10}, {image.cols - 11, 42}, {0, 0, 0}, cv::FILLED);
    cv::putText(image, title, {18, 33}, cv::FONT_HERSHEY_SIMPLEX, 0.62, {255, 255, 255}, 2,
                cv::LINE_AA);

    int y = 60;
    for (const std::string &line : lines) {
        cv::putText(image, line, {18, y}, cv::FONT_HERSHEY_SIMPLEX, 0.48, {220, 220, 220}, 1,
                    cv::LINE_AA);
        y += 18;
    }
}

} // namespace road_segmentation_lab::render
