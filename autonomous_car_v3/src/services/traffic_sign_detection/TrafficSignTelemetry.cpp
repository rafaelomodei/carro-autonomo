#include "services/traffic_sign_detection/TrafficSignTelemetry.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

namespace autonomous_car::services::traffic_sign_detection {
namespace {

std::string jsonEscape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

void appendNumber(std::ostringstream &stream, double value) {
    stream << std::fixed << std::setprecision(6) << value;
}

void appendBoundingBoxJson(std::ostringstream &stream, const TrafficSignBoundingBox &bbox) {
    stream << "{";
    stream << "\"x\":" << bbox.x;
    stream << ",\"y\":" << bbox.y;
    stream << ",\"width\":" << bbox.width;
    stream << ",\"height\":" << bbox.height;
    stream << "}";
}

void appendDetectionJson(std::ostringstream &stream, const TrafficSignDetection &detection) {
    stream << "{";
    stream << "\"sign_id\":\"" << toString(detection.sign_id) << "\"";
    stream << ",\"model_label\":\"" << jsonEscape(detection.model_label) << "\"";
    stream << ",\"display_label\":\"" << jsonEscape(detection.display_label) << "\"";
    stream << ",\"confidence_score\":";
    appendNumber(stream, detection.confidence_score);
    stream << ",\"bbox_frame\":";
    appendBoundingBoxJson(stream, detection.bbox_frame);
    stream << ",\"bbox_roi\":";
    appendBoundingBoxJson(stream, detection.bbox_roi);
    stream << ",\"consecutive_frames\":" << detection.consecutive_frames;
    stream << ",\"required_frames\":" << detection.required_frames;
    stream << ",\"confirmed_at_ms\":" << detection.confirmed_at_ms;
    stream << ",\"last_seen_at_ms\":" << detection.last_seen_at_ms;
    stream << "}";
}

void appendOptionalDetection(std::ostringstream &stream,
                             const std::optional<TrafficSignDetection> &detection) {
    if (!detection.has_value()) {
        stream << "null";
        return;
    }

    appendDetectionJson(stream, *detection);
}

} // namespace

std::string buildTrafficSignTelemetryJson(const TrafficSignFrameResult &result,
                                          std::string_view source) {
    std::ostringstream stream;
    stream << "{";
    stream << "\"type\":\"telemetry.traffic_sign_detection\"";
    stream << ",\"timestamp_ms\":" << result.timestamp_ms;
    stream << ",\"source\":\"" << jsonEscape(source) << "\"";
    stream << ",\"detector_state\":\"" << toString(result.detector_state) << "\"";
    stream << ",\"roi\":{";
    stream << "\"right_width_ratio\":";
    appendNumber(stream, result.roi.right_width_ratio);
    stream << ",\"top_ratio\":";
    appendNumber(stream, result.roi.top_ratio);
    stream << ",\"bottom_ratio\":";
    appendNumber(stream, result.roi.bottom_ratio);
    stream << "}";
    stream << ",\"raw_detections\":[";
    for (std::size_t index = 0; index < result.raw_detections.size(); ++index) {
        if (index > 0) {
            stream << ",";
        }
        appendDetectionJson(stream, result.raw_detections[index]);
    }
    stream << "]";
    stream << ",\"candidate\":";
    appendOptionalDetection(stream, result.candidate);
    stream << ",\"active_detection\":";
    appendOptionalDetection(stream, result.active_detection);
    stream << ",\"last_error\":\"" << jsonEscape(result.last_error) << "\"";
    stream << "}";
    return stream.str();
}

} // namespace autonomous_car::services::traffic_sign_detection
