#include "services/autonomous_control/AutonomousControlDebugRenderer.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::autonomous_control {
namespace {

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
    const std::string &calibration_status, const AutonomousControlSnapshot &snapshot) const {
    cv::Mat segmentation_panel =
        segmentation_renderer_.render(result, config, source_label, calibration_status);
    if (segmentation_panel.empty()) {
        return segmentation_panel;
    }

    cv::Mat control_panel = buildControlPanel(snapshot, {kPanelWidth, segmentation_panel.rows});
    cv::Mat full_panel;
    cv::hconcat(segmentation_panel, control_panel, full_panel);
    return full_panel;
}

cv::Mat AutonomousControlDebugRenderer::buildControlPanel(const AutonomousControlSnapshot &snapshot,
                                                          cv::Size size) {
    cv::Mat panel(size, CV_8UC3, kPanelBackground);
    drawControlStatus(panel, snapshot);
    drawSteeringGauge(panel, snapshot, {22, 132, panel.cols - 44, 130});
    drawTopDownPreview(panel, snapshot, {22, 290, panel.cols - 44, panel.rows - 312});
    cv::rectangle(panel, {0, 0}, {panel.cols - 1, panel.rows - 1}, kPanelBorder, 1, cv::LINE_AA);
    return panel;
}

void AutonomousControlDebugRenderer::drawControlStatus(cv::Mat &panel,
                                                       const AutonomousControlSnapshot &snapshot) {
    cv::rectangle(panel, {18, 18}, {panel.cols - 18, 110}, kPanelSection, cv::FILLED);
    cv::rectangle(panel, {18, 18}, {panel.cols - 18, 110}, kPanelBorder, 1, cv::LINE_AA);

    cv::putText(panel, "Controle autonomo", {32, 48}, cv::FONT_HERSHEY_SIMPLEX, 0.72,
                kTextPrimary, 2, cv::LINE_AA);
    cv::putText(panel, "Modo: " + std::string(toString(snapshot.driving_mode)), {32, 72},
                cv::FONT_HERSHEY_SIMPLEX, 0.5, kTextSecondary, 1, cv::LINE_AA);
    cv::putText(panel, "Estado: " + std::string(toString(snapshot.tracking_state)), {32, 94},
                cv::FONT_HERSHEY_SIMPLEX, 0.52, stateColor(snapshot), 1, cv::LINE_AA);
    cv::putText(panel,
                "Start: " + std::string(snapshot.autonomous_started ? "on" : "off") +
                    " | Motion: " + std::string(toString(snapshot.motion_command)),
                {32, 116}, cv::FONT_HERSHEY_SIMPLEX, 0.45, kTextSecondary, 1, cv::LINE_AA);

    const int x = 32;
    int y = 156;
    const auto draw_line = [&](const std::string &text, const cv::Scalar &color = kTextSecondary) {
        cv::putText(panel, text, {x, y}, cv::FONT_HERSHEY_SIMPLEX, 0.46, color, 1, cv::LINE_AA);
        y += 20;
    };

    if (snapshot.driving_mode == DrivingMode::Manual) {
        draw_line("PID inativo no modo manual", kNeutralColor);
        draw_line("Use o modo autonomous + start");
        return;
    }

    draw_line("Stop: " + std::string(toString(snapshot.stop_reason)));
    draw_line("Conf.: " + formatDouble(snapshot.confidence_score, 2) +
              (snapshot.confidence_ok ? " ok" : " low"),
              snapshot.confidence_ok ? kPositiveColor : kWarningColor);
    draw_line("Erro preview: " + formatDouble(snapshot.preview_error));
    draw_line("Steering cmd: " + formatDouble(snapshot.steering_command),
              stateColor(snapshot));
    draw_line("P/I/D: " + formatDouble(snapshot.pid.proportional) + " / " +
                  formatDouble(snapshot.pid.integral) + " / " +
                  formatDouble(snapshot.pid.derivative),
              kTextPrimary);
    draw_line("Heading: " +
              std::string(snapshot.heading_valid ? formatDouble(snapshot.heading_error_rad)
                                                 : "missing"));
    draw_line("Curv.: " +
              std::string(snapshot.curvature_valid
                              ? formatDouble(snapshot.curvature_indicator_rad)
                              : "missing"));
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
