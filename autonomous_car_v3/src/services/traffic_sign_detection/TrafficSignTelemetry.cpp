#include "services/traffic_sign_detection/TrafficSignTelemetry.hpp"

#include <iomanip>
#include <sstream>

namespace autonomous_car::services::traffic_sign_detection {
namespace {

std::string jsonEscape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (const char ch : value) {
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

void appendBox(std::ostringstream &stream, const DetectionBox &box) {
    stream << "{";
    stream << "\"x\":" << box.x;
    stream << ",\"y\":" << box.y;
    stream << ",\"width\":" << box.width;
    stream << ",\"height\":" << box.height;
    stream << "}";
}

void appendDetection(std::ostringstream &stream, const TrafficSignDetection &detection) {
    stream << "{";
    stream << "\"sign_id\":\"" << jsonEscape(toString(detection.sign_id)) << "\"";
    stream << ",\"model_label\":\"" << jsonEscape(detection.model_label) << "\"";
    stream << ",\"display_label\":\"" << jsonEscape(detection.display_label) << "\"";
    stream << ",\"confidence_score\":";
    appendNumber(stream, detection.confidence_score);
    stream << ",\"bbox_frame\":";
    appendBox(stream, detection.bbox_frame);
    stream << ",\"bbox_roi\":";
    appendBox(stream, detection.bbox_roi);
    stream << "}";
}

void appendStabilized(std::ostringstream &stream,
                      const std::optional<StabilizedTrafficSignDetection> &detection) {
    if (!detection.has_value()) {
        stream << "null";
        return;
    }

    stream << "{";
    stream << "\"sign_id\":\"" << jsonEscape(toString(detection->sign_id)) << "\"";
    stream << ",\"model_label\":\"" << jsonEscape(detection->model_label) << "\"";
    stream << ",\"display_label\":\"" << jsonEscape(detection->display_label) << "\"";
    stream << ",\"confidence_score\":";
    appendNumber(stream, detection->confidence_score);
    stream << ",\"bbox_frame\":";
    appendBox(stream, detection->bbox_frame);
    stream << ",\"bbox_roi\":";
    appendBox(stream, detection->bbox_roi);
    stream << ",\"consecutive_frames\":" << detection->consecutive_frames;
    stream << ",\"required_frames\":" << detection->required_frames;
    stream << ",\"confirmed_at_ms\":";
    if (detection->confirmed_at_ms > 0) {
        stream << detection->confirmed_at_ms;
    } else {
        stream << "null";
    }
    stream << ",\"last_seen_at_ms\":" << detection->last_seen_at_ms;
    stream << "}";
}

} // namespace

std::string buildTrafficSignTelemetryJson(const TrafficSignFrameResult &result,
                                          std::string_view source,
                                          std::int64_t timestamp_ms) {
    std::ostringstream stream;
    stream << "{";
    stream << "\"type\":\"telemetry.traffic_sign_detection\"";
    stream << ",\"timestamp_ms\":" << timestamp_ms;
    stream << ",\"source\":\"" << jsonEscape(source) << "\"";
    stream << ",\"detector_state\":\"" << jsonEscape(toString(result.detector_state)) << "\"";
    stream << ",\"roi\":{";
    stream << "\"x\":" << result.roi.x;
    stream << ",\"y\":" << result.roi.y;
    stream << ",\"width\":" << result.roi.width;
    stream << ",\"height\":" << result.roi.height;
    stream << ",\"right_width_ratio\":";
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
        appendDetection(stream, result.raw_detections[index]);
    }
    stream << "]";
    stream << ",\"candidate\":";
    appendStabilized(stream, result.candidate);
    stream << ",\"active_detection\":";
    appendStabilized(stream, result.active_detection);
    stream << ",\"last_error\":";
    if (result.last_error.empty()) {
        stream << "null";
    } else {
        stream << "\"" << jsonEscape(result.last_error) << "\"";
    }
    stream << "}";
    return stream.str();
}

} // namespace autonomous_car::services::traffic_sign_detection
