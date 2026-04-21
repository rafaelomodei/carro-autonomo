#include "services/autonomous_control/AutonomousControlDebugRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::autonomous_control {
namespace {

namespace vision = autonomous_car::services::vision;

const cv::Scalar kPanelBackground{18, 18, 18};
const cv::Scalar kPanelSection{28, 28, 28};
const cv::Scalar kPanelBorder{78, 78, 78};
const cv::Scalar kTextPrimary{235, 235, 235};
const cv::Scalar kTextSecondary{185, 185, 185};
const cv::Scalar kPositiveColor{60, 220, 120};
const cv::Scalar kNegativeColor{90, 90, 255};
const cv::Scalar kNeutralColor{0, 215, 255};
const cv::Scalar kWarningColor{0, 140, 255};
const cv::Scalar kDangerColor{0, 0, 255};
constexpr int kPanelWidth = 400;

cv::Scalar stateColor(const AutonomousControlSnapshot &snapshot) {
    switch (snapshot.tracking_state) {
    case TrackingState::Tracking:
        return kPositiveColor;
    case TrackingState::Searching:
        return kWarningColor;
    case TrackingState::FailSafe:
        return kDangerColor;
    case TrackingState::Manual:
        return kNeutralColor;
    case TrackingState::Idle:
        return {180, 180, 180};
    }
    return kNeutralColor;
}

std::string formatDoubleLocal(double value, int precision = 3) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

cv::Scalar referenceColor(const ReferenceControlState &reference) {
    if (!reference.valid) {
        return {90, 90, 90};
    }
    return reference.used ? kPositiveColor : kNeutralColor;
}

} // namespace

cv::Mat AutonomousControlDebugRenderer::render(
    const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    const road_segmentation_lab::config::LabConfig &config, const std::string &source_label,
    const std::string &calibration_status, const AutonomousControlSnapshot &snapshot,
    const vision::VisionRuntimeTelemetry &runtime_telemetry) const {
    cv::Mat segmentation_panel =
        segmentation_renderer_.render(result, config, source_label, calibration_status);
    if (segmentation_panel.empty()) {
        return segmentation_panel;
    }

    cv::Mat control_panel =
        buildControlPanel(snapshot, runtime_telemetry, {kPanelWidth, segmentation_panel.rows});
    cv::Mat full_panel;
    cv::hconcat(segmentation_panel, control_panel, full_panel);
    return full_panel;
}

cv::Mat AutonomousControlDebugRenderer::buildControlPanel(
    const AutonomousControlSnapshot &snapshot,
    const vision::VisionRuntimeTelemetry &runtime_telemetry, cv::Size size) {
    cv::Mat panel(size, CV_8UC3, kPanelBackground);
    const int padding = 18;
    const int gap = 12;
    const int inner_width = panel.cols - padding * 2;
    const int control_height = 128;
    const int runtime_height = 120;
    const int gauge_height = 78;
    const int top_down_y = padding + control_height + gap + runtime_height + gap + gauge_height + gap;
    const int top_down_height = std::max(48, panel.rows - top_down_y - padding);

    const cv::Rect control_area(padding, padding, inner_width, control_height);
    const cv::Rect runtime_area(padding, control_area.y + control_area.height + gap, inner_width,
                                runtime_height);
    const cv::Rect gauge_area(padding, runtime_area.y + runtime_area.height + gap, inner_width,
                              gauge_height);
    const cv::Rect top_down_area(padding, top_down_y, inner_width, top_down_height);

    drawControlStatus(panel, snapshot, control_area);
    drawRuntimeStatus(panel, runtime_telemetry, runtime_area);
    drawSteeringGauge(panel, snapshot, gauge_area);
    drawTopDownPreview(panel, snapshot, top_down_area);
    cv::rectangle(panel, {0, 0}, {panel.cols - 1, panel.rows - 1}, kPanelBorder, 1, cv::LINE_AA);
    return panel;
}

void AutonomousControlDebugRenderer::drawControlStatus(cv::Mat &panel,
                                                       const AutonomousControlSnapshot &snapshot,
                                                       const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);

    const cv::Scalar status_color = stateColor(snapshot);
    cv::putText(panel, "Controle autonomo", {area.x + 14, area.y + 24},
                cv::FONT_HERSHEY_SIMPLEX, 0.60, kTextPrimary, 1, cv::LINE_AA);

    const std::vector<std::pair<std::string, cv::Scalar>> lines = {
        {"Modo: " + std::string(autonomous_car::toString(snapshot.driving_mode)) + " | Estado: " +
             std::string(toString(snapshot.tracking_state)),
         status_color},
        {"Autonomo: " + std::string(snapshot.autonomous_started ? "on" : "off") +
             " | Fail-safe: " + std::string(snapshot.fail_safe_active ? "on" : "off"),
         snapshot.fail_safe_active ? kDangerColor : kTextSecondary},
        {"Movimento: " + std::string(toString(snapshot.motion_command)) +
             " | Steering: " + formatDoubleLocal(snapshot.steering_command, 2),
         kTextSecondary},
        {"Confianca: " + formatDoubleLocal(snapshot.confidence_score, 2) +
             " | Pista: " + std::string(snapshot.lane_available ? "ok" : "indisponivel"),
         snapshot.confidence_ok ? kPositiveColor : kWarningColor},
        {"Stop reason: " + std::string(toString(snapshot.stop_reason)),
         snapshot.stop_reason == StopReason::None ? kTextSecondary : kWarningColor},
        {"Heading: " +
             std::string(snapshot.heading_valid ? formatDoubleLocal(snapshot.heading_error_rad, 3)
                                                : "n/a") +
             " | Curv: " +
             std::string(snapshot.curvature_valid
                             ? formatDoubleLocal(snapshot.curvature_indicator_rad, 3)
                             : "n/a"),
         kTextSecondary},
    };

    int y = area.y + 48;
    for (const auto &[line, color] : lines) {
        cv::putText(panel, line, {area.x + 14, y}, cv::FONT_HERSHEY_SIMPLEX, 0.46, color, 1,
                    cv::LINE_AA);
        y += 18;
    }
}

void AutonomousControlDebugRenderer::drawRuntimeStatus(
    cv::Mat &panel, const vision::VisionRuntimeTelemetry &runtime_telemetry,
    const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(panel, "Runtime e stream", {area.x + 14, area.y + 24},
                cv::FONT_HERSHEY_SIMPLEX, 0.60, kTextPrimary, 1, cv::LINE_AA);

    const std::vector<std::string> lines = {
        "Core FPS: " + formatDoubleLocal(runtime_telemetry.core_fps, 1),
        "UI FPS: " + formatDoubleLocal(runtime_telemetry.stream_fps, 1),
        "UI enc: " + formatDoubleLocal(runtime_telemetry.stream_encode_ms, 1) + " ms",
        "Drop UI: " + std::to_string(runtime_telemetry.stream_dropped_frames),
        "Ultima amostra: " + std::to_string(runtime_telemetry.timestamp_ms) + " ms",
    };

    int y = area.y + 48;
    for (const auto &line : lines) {
        cv::putText(panel, line, {area.x + 14, y}, cv::FONT_HERSHEY_SIMPLEX, 0.46,
                    kTextSecondary, 1, cv::LINE_AA);
        y += 18;
    }
}

void AutonomousControlDebugRenderer::drawSteeringGauge(cv::Mat &panel,
                                                       const AutonomousControlSnapshot &snapshot,
                                                       const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(panel, "Direcao", {area.x + 14, area.y + 24}, cv::FONT_HERSHEY_SIMPLEX, 0.60,
                kTextPrimary, 1, cv::LINE_AA);

    const cv::Rect gauge(area.x + 18, area.y + 38, area.width - 36, 18);
    cv::rectangle(panel, gauge, {58, 58, 58}, cv::FILLED);
    cv::rectangle(panel, gauge, kPanelBorder, 1, cv::LINE_AA);

    const int center_x = gauge.x + gauge.width / 2;
    cv::line(panel, {center_x, gauge.y - 3}, {center_x, gauge.y + gauge.height + 3}, kNeutralColor,
             1, cv::LINE_AA);

    const double clamped = std::clamp(snapshot.steering_command, -1.0, 1.0);
    const int indicator_x =
        center_x + static_cast<int>(std::lround(clamped * (gauge.width / 2.0 - 6.0)));
    const cv::Scalar gauge_color = clamped >= 0.0 ? kPositiveColor : kNegativeColor;
    cv::circle(panel, {indicator_x, gauge.y + gauge.height / 2}, 8, gauge_color, cv::FILLED,
               cv::LINE_AA);

    cv::putText(panel, "Erro: " + formatDoubleLocal(snapshot.preview_error, 3),
                {area.x + 18, area.y + 72}, cv::FONT_HERSHEY_SIMPLEX, 0.44, kTextSecondary, 1,
                cv::LINE_AA);
}

void AutonomousControlDebugRenderer::drawTopDownPreview(cv::Mat &panel,
                                                        const AutonomousControlSnapshot &snapshot,
                                                        const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(panel, "Preview da trajetoria", {area.x + 14, area.y + 24},
                cv::FONT_HERSHEY_SIMPLEX, 0.60, kTextPrimary, 1, cv::LINE_AA);

    const cv::Rect map_area(area.x + 18, area.y + 38, area.width - 36, area.height - 56);
    cv::rectangle(panel, map_area, {22, 22, 22}, cv::FILLED);
    cv::rectangle(panel, map_area, {52, 52, 52}, 1, cv::LINE_AA);

    const int center_x = map_area.x + map_area.width / 2;
    cv::line(panel, {center_x, map_area.y}, {center_x, map_area.y + map_area.height}, {56, 56, 56},
             1, cv::LINE_AA);

    auto drawReference = [&](double y_ratio, const ReferenceControlState &reference,
                             const std::string &label) {
        const int y = mapNormalizedY(y_ratio, map_area);
        cv::line(panel, {map_area.x + 14, y}, {map_area.x + map_area.width - 14, y},
                 referenceColor(reference), 1, cv::LINE_AA);
        cv::putText(panel, label, {map_area.x + 10, std::max(map_area.y + 16, y - 6)},
                    cv::FONT_HERSHEY_SIMPLEX, 0.38, referenceColor(reference), 1, cv::LINE_AA);
    };

    drawReference(0.78, snapshot.near_reference, "near");
    drawReference(0.52, snapshot.mid_reference, "mid");
    drawReference(0.28, snapshot.far_reference, "far");

    if (snapshot.projected_path.size() >= 2) {
        for (std::size_t index = 1; index < snapshot.projected_path.size(); ++index) {
            const auto &previous = snapshot.projected_path[index - 1];
            const auto &current = snapshot.projected_path[index];
            cv::line(panel,
                     {mapNormalizedX(previous.x, map_area), mapNormalizedY(previous.y, map_area)},
                     {mapNormalizedX(current.x, map_area), mapNormalizedY(current.y, map_area)},
                     kPositiveColor, 2, cv::LINE_AA);
        }
    }

    const cv::Point vehicle_center(center_x, map_area.y + map_area.height - 18);
    cv::rectangle(panel, {vehicle_center.x - 14, vehicle_center.y - 8, 28, 16}, kNeutralColor,
                  cv::FILLED, cv::LINE_AA);
}

int AutonomousControlDebugRenderer::mapNormalizedX(double x, const cv::Rect &area) {
    const double clamped = std::clamp(x, -1.0, 1.0);
    return area.x + area.width / 2 +
           static_cast<int>(std::lround(clamped * (area.width * 0.35)));
}

int AutonomousControlDebugRenderer::mapNormalizedY(double y, const cv::Rect &area) {
    const double clamped = std::clamp(y, 0.0, 1.0);
    return area.y + area.height -
           static_cast<int>(std::lround(clamped * static_cast<double>(area.height - 12))) - 6;
}

std::string AutonomousControlDebugRenderer::formatDouble(double value, int precision) {
    return formatDoubleLocal(value, precision);
}

} // namespace autonomous_car::services::autonomous_control
