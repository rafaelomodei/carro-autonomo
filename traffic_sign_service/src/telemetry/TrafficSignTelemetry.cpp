#include "telemetry/TrafficSignTelemetry.hpp"

#include <iomanip>
#include <sstream>

namespace traffic_sign_service::telemetry {
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

void appendDetectionJson(std::ostringstream &stream, const SignalDetection &detection) {
    stream << "{";
    stream << "\"sign_id\":\"" << toString(detection.signal_id) << "\"";
    stream << ",\"model_label\":\"" << jsonEscape(detection.model_label) << "\"";
    stream << ",\"display_label\":\"" << jsonEscape(detection.display_label) << "\"";
    stream << ",\"confidence_score\":";
    appendNumber(stream, detection.confidence);
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
                             const std::optional<SignalDetection> &detection) {
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
    stream << "\"left_ratio\":";
    appendNumber(stream, result.roi.left_ratio);
    stream << ",\"right_ratio\":";
    appendNumber(stream, result.roi.right_ratio);
    stream << ",\"top_ratio\":";
    appendNumber(stream, result.roi.top_ratio);
    stream << ",\"bottom_ratio\":";
    appendNumber(stream, result.roi.bottom_ratio);
    stream << ",\"right_width_ratio\":";
    appendNumber(stream, trafficSignRoiRightWidthRatio(result.roi));
    stream << ",\"debug_roi_enabled\":"
           << (result.roi.debug_roi_enabled ? "true" : "false");
    stream << ",\"source_frame_size\":{";
    stream << "\"width\":" << result.roi.source_frame_size.width;
    stream << ",\"height\":" << result.roi.source_frame_size.height;
    stream << "}";
    stream << ",\"frame_rect\":{";
    stream << "\"x\":" << result.roi.frame_rect.x;
    stream << ",\"y\":" << result.roi.frame_rect.y;
    stream << ",\"width\":" << result.roi.frame_rect.width;
    stream << ",\"height\":" << result.roi.frame_rect.height;
    stream << "}";
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

} // namespace traffic_sign_service::telemetry
