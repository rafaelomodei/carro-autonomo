#include "services/vision/VisionRuntimeTelemetry.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

namespace autonomous_car::services::vision {
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

} // namespace

std::string buildVisionRuntimeTelemetryJson(const VisionRuntimeTelemetry &telemetry) {
    std::ostringstream stream;
    stream << "{";
    stream << "\"type\":\"telemetry.vision_runtime\"";
    stream << ",\"timestamp_ms\":" << telemetry.timestamp_ms;
    stream << ",\"source\":\"" << jsonEscape(telemetry.source) << "\"";
    stream << ",\"core_fps\":";
    appendNumber(stream, telemetry.core_fps);
    stream << ",\"stream_fps\":";
    appendNumber(stream, telemetry.stream_fps);
    stream << ",\"traffic_sign_fps\":";
    appendNumber(stream, telemetry.traffic_sign_fps);
    stream << ",\"traffic_sign_inference_ms\":";
    appendNumber(stream, telemetry.traffic_sign_inference_ms);
    stream << ",\"stream_encode_ms\":";
    appendNumber(stream, telemetry.stream_encode_ms);
    stream << ",\"traffic_sign_dropped_frames\":" << telemetry.traffic_sign_dropped_frames;
    stream << ",\"stream_dropped_frames\":" << telemetry.stream_dropped_frames;
    stream << ",\"sign_result_age_ms\":" << telemetry.sign_result_age_ms;
    stream << "}";
    return stream.str();
}

} // namespace autonomous_car::services::vision
