#include "services/road_segmentation/RoadSegmentationTelemetry.hpp"

#include <iomanip>
#include <sstream>

namespace autonomous_car::services::road_segmentation {
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

void appendReference(std::ostringstream &stream,
                     const road_segmentation_lab::pipeline::LookaheadReference &reference) {
    stream << "{";
    stream << "\"valid\":" << (reference.valid ? "true" : "false");
    stream << ",\"point_x\":" << reference.point.x;
    stream << ",\"point_y\":" << reference.point.y;
    stream << ",\"top_y\":" << reference.top_y;
    stream << ",\"bottom_y\":" << reference.bottom_y;
    stream << ",\"center_ratio\":";
    appendNumber(stream, reference.center_ratio);
    stream << ",\"lateral_offset_px\":";
    appendNumber(stream, reference.lateral_offset_px);
    stream << ",\"steering_error_normalized\":";
    appendNumber(stream, reference.steering_error_normalized);
    stream << ",\"sample_count\":" << reference.sample_count;
    stream << "}";
}

} // namespace

std::string buildRoadSegmentationTelemetryJson(
    const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    std::string_view source, std::int64_t timestamp_ms) {
    std::ostringstream stream;
    stream << "{";
    stream << "\"type\":\"telemetry.road_segmentation\"";
    stream << ",\"timestamp_ms\":" << timestamp_ms;
    stream << ",\"source\":\"" << jsonEscape(source) << "\"";
    stream << ",\"lane_found\":" << (result.lane_found ? "true" : "false");
    stream << ",\"confidence_score\":";
    appendNumber(stream, result.confidence_score);
    stream << ",\"lane_center_ratio\":";
    appendNumber(stream, result.lane_center_ratio);
    stream << ",\"steering_error_normalized\":";
    appendNumber(stream, result.steering_error_normalized);
    stream << ",\"lateral_offset_px\":";
    appendNumber(stream, result.lateral_offset_px);
    stream << ",\"heading_error_rad\":";
    appendNumber(stream, result.heading_error_rad);
    stream << ",\"heading_valid\":" << (result.heading_valid ? "true" : "false");
    stream << ",\"curvature_indicator_rad\":";
    appendNumber(stream, result.curvature_indicator_rad);
    stream << ",\"curvature_valid\":" << (result.curvature_valid ? "true" : "false");
    stream << ",\"references\":{";
    stream << "\"near\":";
    appendReference(stream, result.near_reference);
    stream << ",\"mid\":";
    appendReference(stream, result.mid_reference);
    stream << ",\"far\":";
    appendReference(stream, result.far_reference);
    stream << "}";
    stream << "}";
    return stream.str();
}

} // namespace autonomous_car::services::road_segmentation
