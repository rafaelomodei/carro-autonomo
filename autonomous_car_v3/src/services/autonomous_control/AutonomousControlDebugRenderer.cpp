#include "services/autonomous_control/AutonomousControlDebugRenderer.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::autonomous_control {
namespace {

namespace ts = autonomous_car::services::traffic_sign_detection;
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
constexpr int kPanelWidth = 360;

cv::Scalar trafficStateColor(ts::TrafficSignDetectorState state) {
    switch (state) {
    case ts::TrafficSignDetectorState::Disabled:
        return {170, 170, 170};
    case ts::TrafficSignDetectorState::Idle:
        return kNeutralColor;
    case ts::TrafficSignDetectorState::Candidate:
        return kWarningColor;
    case ts::TrafficSignDetectorState::Confirmed:
        return kPositiveColor;
    case ts::TrafficSignDetectorState::Error:
        return kDangerColor;
    }

    return kDangerColor;
}

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

} // namespace

cv::Mat AutonomousControlDebugRenderer::render(
    const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    const road_segmentation_lab::config::LabConfig &config, const std::string &source_label,
    const std::string &calibration_status, const AutonomousControlSnapshot &snapshot,
    const vision::VisionRuntimeTelemetry &runtime_telemetry,
    const ts::TrafficSignFrameResult &traffic_sign_result) const {
    cv::Mat segmentation_panel =
        segmentation_renderer_.render(result, config, source_label, calibration_status);
    if (segmentation_panel.empty()) {
        return segmentation_panel;
    }

    cv::Mat control_panel = buildControlPanel(snapshot, runtime_telemetry, traffic_sign_result,
                                              {kPanelWidth, segmentation_panel.rows});
    cv::Mat full_panel;
    cv::hconcat(segmentation_panel, control_panel, full_panel);
    return full_panel;
}

cv::Mat AutonomousControlDebugRenderer::buildControlPanel(
    const AutonomousControlSnapshot &snapshot,
    const vision::VisionRuntimeTelemetry &runtime_telemetry,
    const ts::TrafficSignFrameResult &traffic_sign_result,
    cv::Size size) {
    cv::Mat panel(size, CV_8UC3, kPanelBackground);
    const int padding = 18;
    const int inner_width = panel.cols - padding * 2;
    const cv::Rect control_area(padding, padding, inner_width, 108);
    const cv::Rect traffic_area(padding, control_area.y + control_area.height + 12, inner_width,
                                152);
    const cv::Rect gauge_area(padding, traffic_area.y + traffic_area.height + 12, inner_width,
                              96);
    const int top_down_y = gauge_area.y + gauge_area.height + 12;
    const int top_down_height = std::max(96, panel.rows - top_down_y - padding);

    drawControlStatus(panel, snapshot, control_area);
    drawTrafficSignStatus(panel, runtime_telemetry, traffic_sign_result, traffic_area);
    drawSteeringGauge(panel, snapshot, gauge_area);
    drawTopDownPreview(panel, snapshot,
                       {padding, top_down_y, inner_width, top_down_height});
    cv::rectangle(panel, {0, 0}, {panel.cols - 1, panel.rows - 1}, kPanelBorder, 1, cv::LINE_AA);
    return panel;
}

void AutonomousControlDebugRenderer::drawControlStatus(cv::Mat &panel,
                                                       const AutonomousControlSnapshot &snapshot,
                                                       const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(panel, "Controle autonomo", {area.x + 14, area.y + 26},
                cv::FONT_HERSHEY_SIMPLEX, 0.64,
                kTextPrimary, 2, cv::LINE_AA);
    cv::putText(panel, "Modo: " + std::string(toString(snapshot.driving_mode)),
                {area.x + 14, area.y + 46}, cv::FONT_HERSHEY_SIMPLEX, 0.46, kTextSecondary, 1,
                cv::LINE_AA);
    cv::putText(panel, "Estado: " + std::string(toString(snapshot.tracking_state)),
                {area.x + 14, area.y + 66}, cv::FONT_HERSHEY_SIMPLEX, 0.48,
                stateColor(snapshot), 1, cv::LINE_AA);
    cv::putText(panel,
                "Start: " + std::string(snapshot.autonomous_started ? "on" : "off") +
                    " | Motion: " + std::string(toString(snapshot.motion_command)),
                {area.x + 14, area.y + 86}, cv::FONT_HERSHEY_SIMPLEX, 0.40, kTextSecondary, 1,
                cv::LINE_AA);

    if (snapshot.driving_mode == DrivingMode::Manual) {
        cv::putText(panel, "PID inativo no modo manual", {area.x + 14, area.y + 104},
                    cv::FONT_HERSHEY_SIMPLEX, 0.38, kNeutralColor, 1, cv::LINE_AA);
        return;
    }

    cv::putText(panel,
                "Conf.: " + formatDouble(snapshot.confidence_score, 2) +
                    " | Steering: " + formatDouble(snapshot.steering_command, 2),
                {area.x + 14, area.y + 104}, cv::FONT_HERSHEY_SIMPLEX, 0.38,
                snapshot.confidence_ok ? kPositiveColor : kWarningColor, 1, cv::LINE_AA);
}

void AutonomousControlDebugRenderer::drawTrafficSignStatus(
    cv::Mat &panel, const vision::VisionRuntimeTelemetry &runtime_telemetry,
    const ts::TrafficSignFrameResult &traffic_sign_result,
    const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);
    cv::putText(panel, "Sinalizacao", {area.x + 14, area.y + 26}, cv::FONT_HERSHEY_SIMPLEX,
                0.6, kTextPrimary, 1, cv::LINE_AA);
    cv::putText(panel,
                "Estado: " +
                    std::string(ts::toString(traffic_sign_result.detector_state)),
                {area.x + 14, area.y + 46}, cv::FONT_HERSHEY_SIMPLEX, 0.45,
                trafficStateColor(traffic_sign_result.detector_state), 1, cv::LINE_AA);
    cv::putText(panel,
                "ROI: " + formatDouble(traffic_sign_result.roi.right_width_ratio, 2) +
                    " | " + formatDouble(traffic_sign_result.roi.top_ratio, 2) + " -> " +
                    formatDouble(traffic_sign_result.roi.bottom_ratio, 2),
                {area.x + 14, area.y + 66}, cv::FONT_HERSHEY_SIMPLEX, 0.38, kTextSecondary, 1,
                cv::LINE_AA);
    cv::putText(panel,
                "Brutas: " + std::to_string(traffic_sign_result.raw_detections.size()),
                {area.x + 14, area.y + 84}, cv::FONT_HERSHEY_SIMPLEX, 0.38, kTextSecondary, 1,
                cv::LINE_AA);
    cv::putText(panel,
                "FPS core/stream/sign: " + formatDouble(runtime_telemetry.core_fps, 1) + " / " +
                    formatDouble(runtime_telemetry.stream_fps, 1) + " / " +
                    formatDouble(runtime_telemetry.traffic_sign_fps, 1),
                {area.x + 14, area.y + 102}, cv::FONT_HERSHEY_SIMPLEX, 0.38, kTextSecondary, 1,
                cv::LINE_AA);
    cv::putText(panel,
                "Infer/encode ms: " +
                    formatDouble(runtime_telemetry.traffic_sign_inference_ms, 1) + " / " +
                    formatDouble(runtime_telemetry.stream_encode_ms, 1),
                {area.x + 14, area.y + 120}, cv::FONT_HERSHEY_SIMPLEX, 0.38, kTextSecondary, 1,
                cv::LINE_AA);
    cv::putText(panel,
                "Drop sign/stream: " +
                    std::to_string(runtime_telemetry.traffic_sign_dropped_frames) + " / " +
                    std::to_string(runtime_telemetry.stream_dropped_frames),
                {area.x + 14, area.y + 138}, cv::FONT_HERSHEY_SIMPLEX, 0.38, kTextSecondary, 1,
                cv::LINE_AA);
    std::string summary = "Active: none";
    cv::Scalar summary_color = kTextSecondary;

    if (traffic_sign_result.active_detection.has_value()) {
        summary = "Active: " + traffic_sign_result.active_detection->display_label + " " +
                  formatDouble(traffic_sign_result.active_detection->confidence_score, 2);
        summary_color = kPositiveColor;
    } else if (traffic_sign_result.candidate.has_value()) {
        summary = "Candidate: " + traffic_sign_result.candidate->display_label + " " +
                  formatDouble(traffic_sign_result.candidate->confidence_score, 2);
        summary_color = kWarningColor;
    }

    if (!traffic_sign_result.last_error.empty()) {
        summary = "Erro: " + traffic_sign_result.last_error;
        summary_color = kDangerColor;
    }

    cv::putText(panel, summary, {area.x + 14, area.y + 148}, cv::FONT_HERSHEY_SIMPLEX, 0.38,
                summary_color, 1, cv::LINE_AA);
}

void AutonomousControlDebugRenderer::drawSteeringGauge(cv::Mat &panel,
                                                       const AutonomousControlSnapshot &snapshot,
                                                       const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);
    cv::putText(panel, "Comando de direcao", {area.x + 14, area.y + 24},
                cv::FONT_HERSHEY_SIMPLEX, 0.52, kTextPrimary, 1, cv::LINE_AA);

    const int center_y = area.y + area.height / 2 + 8;
    const int center_x = area.x + area.width / 2;
    const int half_width = (area.width - 60) / 2;

    cv::line(panel, {center_x - half_width, center_y}, {center_x + half_width, center_y},
             {110, 110, 110}, 2, cv::LINE_AA);
    cv::line(panel, {center_x, center_y - 24}, {center_x, center_y + 24}, kNeutralColor, 1,
             cv::LINE_AA);

    const double command = std::clamp(snapshot.steering_command, -1.0, 1.0);
    const int arrow_x = center_x + static_cast<int>(std::round(command * half_width));
    const cv::Scalar arrow_color =
        command < 0.0 ? kNegativeColor : (command > 0.0 ? kPositiveColor : kNeutralColor);

    cv::arrowedLine(panel, {center_x, center_y}, {arrow_x, center_y}, arrow_color, 4,
                    cv::LINE_AA, 0, 0.18);
    cv::putText(panel, "L", {area.x + 18, center_y + 6}, cv::FONT_HERSHEY_SIMPLEX, 0.56,
                kNegativeColor, 1, cv::LINE_AA);
    cv::putText(panel, "R", {area.x + area.width - 32, center_y + 6}, cv::FONT_HERSHEY_SIMPLEX,
                0.56, kPositiveColor, 1, cv::LINE_AA);
}

void AutonomousControlDebugRenderer::drawTopDownPreview(
    cv::Mat &panel, const AutonomousControlSnapshot &snapshot, const cv::Rect &area) {
    cv::rectangle(panel, area, kPanelSection, cv::FILLED);
    cv::rectangle(panel, area, kPanelBorder, 1, cv::LINE_AA);
    cv::putText(panel, "Mapa superior / previsao", {area.x + 14, area.y + 24},
                cv::FONT_HERSHEY_SIMPLEX, 0.52, kTextPrimary, 1, cv::LINE_AA);

    const cv::Point vehicle_center(area.x + area.width / 2, area.y + area.height - 30);
    const std::vector<cv::Point> vehicle_polygon = {
        {vehicle_center.x, vehicle_center.y - 18},
        {vehicle_center.x - 14, vehicle_center.y + 12},
        {vehicle_center.x + 14, vehicle_center.y + 12},
    };
    cv::fillConvexPoly(panel, vehicle_polygon, {220, 220, 220}, cv::LINE_AA);

    cv::line(panel, {vehicle_center.x, area.y + 36}, {vehicle_center.x, vehicle_center.y - 28},
             {100, 100, 100}, 1, cv::LINE_AA);

    const auto draw_reference = [&](const ReferenceControlState &reference, double y_ratio,
                                    const std::string &label, const cv::Scalar &color) {
        if (!reference.valid) {
            return;
        }

        const int x = mapNormalizedX(reference.steering_error_normalized, area);
        const int y = mapNormalizedY(y_ratio, area);
        cv::circle(panel, {x, y}, 5, color, cv::FILLED, cv::LINE_AA);
        cv::putText(panel, label, {x + 8, y - 6}, cv::FONT_HERSHEY_SIMPLEX, 0.42, color, 1,
                    cv::LINE_AA);
    };

    draw_reference(snapshot.far_reference, 0.2, "FAR", {90, 90, 255});
    draw_reference(snapshot.mid_reference, 0.48, "MID", {255, 180, 0});
    draw_reference(snapshot.near_reference, 0.75, "NEAR", {60, 220, 120});

    if (snapshot.projected_path.size() >= 2) {
        std::vector<cv::Point> path;
        path.reserve(snapshot.projected_path.size());
        for (const TrajectoryPoint &point : snapshot.projected_path) {
            path.emplace_back(mapNormalizedX(point.x, area),
                              mapNormalizedY(1.0 - point.y * 0.92, area));
        }
        cv::polylines(panel, path, false, stateColor(snapshot), 3, cv::LINE_AA);
    }
}

int AutonomousControlDebugRenderer::mapNormalizedX(double x, const cv::Rect &area) {
    const double clamped = std::clamp(x, -1.0, 1.0);
    const double ratio = (clamped + 1.0) / 2.0;
    return area.x + 24 +
           static_cast<int>(std::round(ratio * static_cast<double>(area.width - 48)));
}

int AutonomousControlDebugRenderer::mapNormalizedY(double y, const cv::Rect &area) {
    const double clamped = std::clamp(y, 0.0, 1.0);
    return area.y + 36 +
           static_cast<int>(std::round(clamped * static_cast<double>(area.height - 72)));
}

std::string AutonomousControlDebugRenderer::formatDouble(double value, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

} // namespace autonomous_car::services::autonomous_control
