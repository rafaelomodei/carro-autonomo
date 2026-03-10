#include "services/autonomous_control/AutonomousControlTelemetry.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

namespace autonomous_car::services::autonomous_control {
namespace {

void appendNumber(std::ostringstream &stream, double value) {
    stream << std::fixed << std::setprecision(6) << value;
}

void appendReference(std::ostringstream &stream, const ReferenceControlState &reference) {
    stream << "{";
    stream << "\"valid\":" << (reference.valid ? "true" : "false");
    stream << ",\"used\":" << (reference.used ? "true" : "false");
    stream << ",\"configured_weight\":";
    appendNumber(stream, reference.configured_weight);
    stream << ",\"applied_weight\":";
    appendNumber(stream, reference.applied_weight);
    stream << ",\"steering_error_normalized\":";
    appendNumber(stream, reference.steering_error_normalized);
    stream << ",\"lateral_offset_px\":";
    appendNumber(stream, reference.lateral_offset_px);
    stream << "}";
}

} // namespace

std::string buildAutonomousControlTelemetryJson(const AutonomousControlSnapshot &snapshot) {
    std::ostringstream stream;
    stream << "{";
    stream << "\"type\":\"telemetry.autonomous_control\"";
    stream << ",\"timestamp_ms\":" << snapshot.timestamp_ms;
    stream << ",\"driving_mode\":\"" << toString(snapshot.driving_mode) << "\"";
    stream << ",\"autonomous_started\":" << (snapshot.autonomous_started ? "true" : "false");
    stream << ",\"tracking_state\":\"" << toString(snapshot.tracking_state) << "\"";
    stream << ",\"stop_reason\":\"" << toString(snapshot.stop_reason) << "\"";
    stream << ",\"fail_safe_active\":" << (snapshot.fail_safe_active ? "true" : "false");
    stream << ",\"lane_available\":" << (snapshot.lane_available ? "true" : "false");
    stream << ",\"confidence_ok\":" << (snapshot.confidence_ok ? "true" : "false");
    stream << ",\"confidence_score\":";
    appendNumber(stream, snapshot.confidence_score);
    stream << ",\"preview_error\":";
    appendNumber(stream, snapshot.preview_error);
    stream << ",\"heading_error_rad\":";
    appendNumber(stream, snapshot.heading_error_rad);
    stream << ",\"heading_valid\":" << (snapshot.heading_valid ? "true" : "false");
    stream << ",\"curvature_indicator_rad\":";
    appendNumber(stream, snapshot.curvature_indicator_rad);
    stream << ",\"curvature_valid\":" << (snapshot.curvature_valid ? "true" : "false");
    stream << ",\"references\":{";
    stream << "\"near\":";
    appendReference(stream, snapshot.near_reference);
    stream << ",\"mid\":";
    appendReference(stream, snapshot.mid_reference);
    stream << ",\"far\":";
    appendReference(stream, snapshot.far_reference);
    stream << "}";
    stream << ",\"pid\":{";
    stream << "\"error\":";
    appendNumber(stream, snapshot.pid.error);
    stream << ",\"p\":";
    appendNumber(stream, snapshot.pid.proportional);
    stream << ",\"i\":";
    appendNumber(stream, snapshot.pid.integral);
    stream << ",\"d\":";
    appendNumber(stream, snapshot.pid.derivative);
    stream << ",\"raw_output\":";
    appendNumber(stream, snapshot.pid.raw_output);
    stream << ",\"output\":";
    appendNumber(stream, snapshot.pid.pid_output);
    stream << "}";
    stream << ",\"steering_command\":";
    appendNumber(stream, snapshot.steering_command);
    stream << ",\"motion_command\":\"" << toString(snapshot.motion_command) << "\"";
    stream << ",\"projected_path\":[";
    for (std::size_t index = 0; index < snapshot.projected_path.size(); ++index) {
        if (index > 0) {
            stream << ",";
        }
        stream << "{";
        stream << "\"x\":";
        appendNumber(stream, snapshot.projected_path[index].x);
        stream << ",\"y\":";
        appendNumber(stream, snapshot.projected_path[index].y);
        stream << "}";
    }
    stream << "]";
    stream << "}";
    return stream.str();
}

} // namespace autonomous_car::services::autonomous_control
