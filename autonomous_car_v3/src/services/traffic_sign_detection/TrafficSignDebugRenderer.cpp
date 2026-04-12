#include "services/traffic_sign_detection/TrafficSignDebugRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::traffic_sign_detection {
namespace {

namespace vision = autonomous_car::services::vision;

const cv::Scalar kBackground{18, 18, 18};
const cv::Scalar kPanelBackground{28, 28, 28};
const cv::Scalar kPanelBorder{78, 78, 78};
const cv::Scalar kTextPrimary{235, 235, 235};
const cv::Scalar kTextSecondary{185, 185, 185};
const cv::Scalar kPositiveColor{60, 220, 120};
const cv::Scalar kWarningColor{0, 140, 255};
const cv::Scalar kDangerColor{0, 0, 255};
const cv::Scalar kNeutralColor{0, 215, 255};
const cv::Scalar kRoiColor{255, 200, 0};
const cv::Scalar kRawColor{0, 215, 255};
const cv::Scalar kCandidateColor{0, 165, 255};
const cv::Scalar kActiveColor{60, 220, 120};
const cv::Scalar kLabelBackground{16, 16, 16};

constexpr int kCanvasWidth = 1220;
constexpr int kCanvasHeight = 760;
constexpr int kPadding = 18;
constexpr int kGap = 18;
constexpr int kHeaderHeight = 170;
constexpr int kFooterHeight = 150;

cv::Scalar trafficStateColor(TrafficSignDetectorState state) {
    switch (state) {
    case TrafficSignDetectorState::Disabled:
        return {170, 170, 170};
    case TrafficSignDetectorState::Idle:
        return kNeutralColor;
    case TrafficSignDetectorState::Candidate:
        return kWarningColor;
    case TrafficSignDetectorState::Confirmed:
        return kPositiveColor;
    case TrafficSignDetectorState::Error:
        return kDangerColor;
    }

    return kDangerColor;
}

std::string formatDouble(double value, int precision = 2) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string formatAgeLabel(std::int64_t age_ms) {
    return age_ms >= 0 ? std::to_string(age_ms) + " ms" : "n/a";
}

std::vector<std::string> wrapText(const std::string &text, std::size_t max_chars) {
    if (text.empty()) {
        return {""};
    }

    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string current_line;

    while (stream >> word) {
        if (current_line.empty()) {
            current_line = word;
            continue;
        }

        if (current_line.size() + 1 + word.size() <= max_chars) {
            current_line += " " + word;
        } else {
            lines.push_back(current_line);
            current_line = word;
        }
    }

    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    return lines.empty() ? std::vector<std::string>{text} : lines;
}

cv::Mat ensureBgr(const cv::Mat &image) {
    if (image.empty()) {
        return {};
    }

    if (image.channels() == 3) {
        return image;
    }

    cv::Mat converted;
    if (image.channels() == 1) {
        cv::cvtColor(image, converted, cv::COLOR_GRAY2BGR);
    } else if (image.channels() == 4) {
        cv::cvtColor(image, converted, cv::COLOR_BGRA2BGR);
    } else {
        converted = image.clone();
    }
    return converted;
}

cv::Mat fitImageToCanvas(const cv::Mat &source, cv::Size canvas_size, cv::Rect *placed_rect) {
    cv::Mat canvas(canvas_size, CV_8UC3, cv::Scalar{10, 10, 10});
    if (placed_rect) {
        *placed_rect = {};
    }

    if (source.empty() || canvas_size.width <= 0 || canvas_size.height <= 0) {
        return canvas;
    }

    const cv::Mat color_source = ensureBgr(source);
    const double scale = std::min(static_cast<double>(canvas_size.width) / color_source.cols,
                                  static_cast<double>(canvas_size.height) / color_source.rows);
    const int resized_width = std::max(1, static_cast<int>(std::lround(color_source.cols * scale)));
    const int resized_height =
        std::max(1, static_cast<int>(std::lround(color_source.rows * scale)));

    cv::Mat resized;
    cv::resize(color_source, resized, {resized_width, resized_height}, 0.0, 0.0,
               source.channels() == 1 ? cv::INTER_NEAREST : cv::INTER_LINEAR);

    const int offset_x = (canvas_size.width - resized_width) / 2;
    const int offset_y = (canvas_size.height - resized_height) / 2;
    resized.copyTo(canvas(cv::Rect(offset_x, offset_y, resized_width, resized_height)));
    if (placed_rect) {
        *placed_rect = cv::Rect(offset_x, offset_y, resized_width, resized_height);
    }
    return canvas;
}

void drawLabelTag(cv::Mat &image, const cv::Point &anchor, const std::string &text,
                  const cv::Scalar &color) {
    const cv::Size text_size =
        cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, nullptr);
    const int baseline_y = std::clamp(anchor.y, 18, std::max(18, image.rows - 6));
    const cv::Rect background(anchor.x, baseline_y - text_size.height - 10,
                              text_size.width + 12, text_size.height + 10);
    const cv::Rect clipped = background & cv::Rect(0, 0, image.cols, image.rows);
    if (clipped.area() <= 0) {
        return;
    }

    cv::rectangle(image, clipped, kLabelBackground, cv::FILLED, cv::LINE_AA);
    cv::rectangle(image, clipped, color, 1, cv::LINE_AA);
    cv::putText(image, text, {clipped.x + 6, clipped.y + clipped.height - 6},
                cv::FONT_HERSHEY_SIMPLEX, 0.45, color, 1, cv::LINE_AA);
}

void drawDetectionBoxOnRoiTile(cv::Mat &image, const TrafficSignDetection &detection,
                               const cv::Size &source_size, const cv::Rect &content_rect,
                               const cv::Scalar &color, int thickness,
                               const std::string &prefix = {}) {
    if (image.empty() || content_rect.area() <= 0 || source_size.width <= 0 ||
        source_size.height <= 0) {
        return;
    }

    TrafficSignBoundingBox scaled_box = scaleBoundingBox(detection.bbox_roi, source_size,
                                                         content_rect.size());
    scaled_box = clampBoundingBox(
        translateBoundingBox(scaled_box, content_rect.tl()),
        cv::Rect(0, 0, image.cols, image.rows));

    const cv::Rect rect = toCvRect(scaled_box) & cv::Rect(0, 0, image.cols, image.rows);
    if (rect.area() <= 0) {
        return;
    }

    cv::rectangle(image, rect, color, thickness, cv::LINE_AA);
    const std::string tag =
        prefix.empty()
            ? detection.display_label + " " + formatDouble(detection.confidence_score, 2)
            : prefix + ": " + detection.display_label + " " +
                  formatDouble(detection.confidence_score, 2);
    drawLabelTag(image, {rect.x, std::max(18, rect.y - 4)}, tag, color);
}

void drawTileFrame(cv::Mat &canvas, const cv::Rect &area, const std::string &title) {
    cv::rectangle(canvas, area, kPanelBackground, cv::FILLED);
    cv::rectangle(canvas, area, kPanelBorder, 1, cv::LINE_AA);
    cv::putText(canvas, title, {area.x + 14, area.y + 24}, cv::FONT_HERSHEY_SIMPLEX, 0.60,
                kTextPrimary, 1, cv::LINE_AA);
}

std::string activeSummary(const TrafficSignFrameResult &result) {
    if (result.active_detection.has_value()) {
        return result.active_detection->display_label + " " +
               formatDouble(result.active_detection->confidence_score, 2);
    }

    if (result.candidate.has_value()) {
        return "Cand. " + result.candidate->display_label + " " +
               formatDouble(result.candidate->confidence_score, 2);
    }

    return "nenhuma";
}

} // namespace

cv::Mat TrafficSignDebugRenderer::render(const TrafficSignFrameResult &result,
                                         const vision::VisionRuntimeTelemetry &runtime_telemetry) const {
    cv::Mat canvas(kCanvasHeight, kCanvasWidth, CV_8UC3, kBackground);

    const cv::Rect header_area(kPadding, kPadding, canvas.cols - kPadding * 2, kHeaderHeight);
    const int tile_y = header_area.y + header_area.height + kGap;
    const int tile_height = canvas.rows - tile_y - kFooterHeight - kGap - kPadding;
    const int tile_width = (canvas.cols - kPadding * 2 - kGap) / 2;
    const cv::Rect roi_tile_area(kPadding, tile_y, tile_width, tile_height);
    const cv::Rect model_tile_area(roi_tile_area.x + roi_tile_area.width + kGap, tile_y,
                                   tile_width, tile_height);
    const cv::Rect footer_area(kPadding, roi_tile_area.y + roi_tile_area.height + kGap,
                               canvas.cols - kPadding * 2, kFooterHeight);

    cv::rectangle(canvas, header_area, kPanelBackground, cv::FILLED);
    cv::rectangle(canvas, header_area, kPanelBorder, 1, cv::LINE_AA);
    cv::rectangle(canvas, footer_area, kPanelBackground, cv::FILLED);
    cv::rectangle(canvas, footer_area, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(canvas, "Traffic Sign Debug", {header_area.x + 16, header_area.y + 28},
                cv::FONT_HERSHEY_SIMPLEX, 0.74, kTextPrimary, 2, cv::LINE_AA);
    cv::putText(canvas,
                "ROI original + input preprocessado enviado ao Edge Impulse",
                {header_area.x + 16, header_area.y + 52}, cv::FONT_HERSHEY_SIMPLEX, 0.44,
                kTextSecondary, 1, cv::LINE_AA);

    const cv::Scalar state_color = trafficStateColor(result.detector_state);
    const std::vector<std::string> header_lines = {
        "Estado: " + std::string(toString(result.detector_state)) + " | ativa/cand.: " +
            activeSummary(result),
        "Deteccoes brutas: " + std::to_string(result.raw_detections.size()) +
            " | Placa FPS: " + formatDouble(runtime_telemetry.traffic_sign_fps, 1) +
            " | Inferencia: " + formatDouble(runtime_telemetry.traffic_sign_inference_ms, 1) +
            " ms | idade: " + formatAgeLabel(runtime_telemetry.sign_result_age_ms),
        "ROI % X: " + formatDouble(result.roi.left_ratio, 2) + " -> " +
            formatDouble(result.roi.right_ratio, 2) + " | Y: " +
            formatDouble(result.roi.top_ratio, 2) + " -> " +
            formatDouble(result.roi.bottom_ratio, 2),
        "ROI px: x=" + std::to_string(result.roi.frame_rect.x) + " y=" +
            std::to_string(result.roi.frame_rect.y) + " w=" +
            std::to_string(result.roi.frame_rect.width) + " h=" +
            std::to_string(result.roi.frame_rect.height) + " | overlay: " +
            std::string(result.roi.debug_roi_enabled ? "on" : "off"),
    };

    int line_y = header_area.y + 82;
    for (std::size_t index = 0; index < header_lines.size(); ++index) {
        cv::putText(canvas, header_lines[index], {header_area.x + 16, line_y},
                    cv::FONT_HERSHEY_SIMPLEX, 0.42, index == 0 ? state_color : kTextSecondary, 1,
                    cv::LINE_AA);
        line_y += 22;
    }

    if (!result.last_error.empty()) {
        for (const auto &line : wrapText("Erro: " + result.last_error, 118)) {
            cv::putText(canvas, line, {header_area.x + 16, line_y}, cv::FONT_HERSHEY_SIMPLEX,
                        0.42, kDangerColor, 1, cv::LINE_AA);
            line_y += 20;
        }
    }

    drawTileFrame(canvas, roi_tile_area, "ROI original do detector");
    drawTileFrame(canvas, model_tile_area, "Input preprocessado do modelo");

    const cv::Rect roi_content(roi_tile_area.x + 12, roi_tile_area.y + 36,
                               roi_tile_area.width - 24, roi_tile_area.height - 48);
    const cv::Rect model_content(model_tile_area.x + 12, model_tile_area.y + 36,
                                 model_tile_area.width - 24, model_tile_area.height - 48);

    cv::Rect roi_image_rect;
    cv::Mat roi_canvas = fitImageToCanvas(result.debug_roi_frame, roi_content.size(), &roi_image_rect);
    roi_canvas.copyTo(canvas(roi_content));
    if (!result.debug_roi_frame.empty()) {
        const cv::Rect roi_bounds(roi_content.x + roi_image_rect.x, roi_content.y + roi_image_rect.y,
                                  roi_image_rect.width, roi_image_rect.height);
        const cv::Size source_size = result.debug_roi_frame.size();
        for (const auto &detection : result.raw_detections) {
            drawDetectionBoxOnRoiTile(canvas, detection, source_size, roi_bounds, kRawColor, 1);
        }
        if (result.candidate.has_value()) {
            drawDetectionBoxOnRoiTile(canvas, *result.candidate, source_size, roi_bounds,
                                      kCandidateColor, 2, "Candidate");
        }
        if (result.active_detection.has_value()) {
            drawDetectionBoxOnRoiTile(canvas, *result.active_detection, source_size, roi_bounds,
                                      kActiveColor, 3, "Active");
        }
        drawLabelTag(canvas, {roi_content.x + 10, roi_content.y + 22},
                     std::to_string(source_size.width) + "x" +
                         std::to_string(source_size.height),
                     kRoiColor);
    } else {
        cv::putText(canvas, "Aguardando ROI da inferencia", {roi_content.x + 18, roi_content.y + 42},
                    cv::FONT_HERSHEY_SIMPLEX, 0.48, kTextSecondary, 1, cv::LINE_AA);
    }

    cv::Rect model_image_rect;
    cv::Mat model_canvas =
        fitImageToCanvas(result.debug_model_input_frame, model_content.size(), &model_image_rect);
    model_canvas.copyTo(canvas(model_content));
    if (!result.debug_model_input_frame.empty()) {
        drawLabelTag(canvas, {model_content.x + 10, model_content.y + 22},
                     std::to_string(result.debug_model_input_frame.cols) + "x" +
                         std::to_string(result.debug_model_input_frame.rows),
                     kNeutralColor);
    } else {
        cv::putText(canvas, "Aguardando input preprocessado",
                    {model_content.x + 18, model_content.y + 42}, cv::FONT_HERSHEY_SIMPLEX, 0.48,
                    kTextSecondary, 1, cv::LINE_AA);
    }

    cv::putText(canvas, "Diagnostico do modelo compilado", {footer_area.x + 16, footer_area.y + 28},
                cv::FONT_HERSHEY_SIMPLEX, 0.58, kTextPrimary, 1, cv::LINE_AA);
    int footer_y = footer_area.y + 54;
    const std::vector<std::string> footer_blocks = {
        "Labels compiladas: " +
            (result.model_labels_summary.empty() ? std::string("n/a") : result.model_labels_summary),
        "Runtime usa o modelo embutido em third_party/edge_impulse/traffic_sign_model.",
        "O zip em edgeImpulse/ nao e carregado em runtime pelo binario atual.",
    };
    for (const auto &block : footer_blocks) {
        for (const auto &line : wrapText(block, 116)) {
            cv::putText(canvas, line, {footer_area.x + 16, footer_y}, cv::FONT_HERSHEY_SIMPLEX,
                        0.42, kTextSecondary, 1, cv::LINE_AA);
            footer_y += 20;
        }
    }

    cv::rectangle(canvas, {0, 0}, {canvas.cols - 1, canvas.rows - 1}, kPanelBorder, 1, cv::LINE_AA);
    return canvas;
}

} // namespace autonomous_car::services::traffic_sign_detection
