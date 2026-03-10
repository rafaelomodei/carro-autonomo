#include <cstdint>
#include <string>

#include "TestRegistry.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/road_segmentation/RoadSegmentationTelemetry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expectContains;
using road_segmentation_lab::pipeline::LookaheadReference;
using road_segmentation_lab::pipeline::RoadSegmentationResult;

void testTelemetrySerialization() {
    RoadSegmentationResult result;
    result.lane_found = true;
    result.confidence_score = 0.87;
    result.lane_center_ratio = 0.52;
    result.steering_error_normalized = 0.11;
    result.lateral_offset_px = 14.5;
    result.heading_error_rad = 0.23;
    result.heading_valid = true;
    result.curvature_indicator_rad = -0.07;
    result.curvature_valid = true;

    result.near_reference = LookaheadReference{{160, 200}, 150, 220, 0.55, 18.0, 0.14, 12, true};
    result.mid_reference = LookaheadReference{{155, 160}, 100, 170, 0.52, 10.0, 0.08, 10, true};
    result.far_reference = LookaheadReference{{150, 120}, 50, 120, 0.50, 2.0, 0.01, 9, true};

    const std::string json = autonomous_car::services::road_segmentation::
        buildRoadSegmentationTelemetryJson(result, "Camera index 0", 123456789);

    expectContains(json, "\"type\":\"telemetry.road_segmentation\"",
                   "Payload deve identificar o tipo de telemetria.");
    expectContains(json, "\"timestamp_ms\":123456789",
                   "Payload deve carregar timestamp.");
    expectContains(json, "\"source\":\"Camera index 0\"",
                   "Payload deve carregar a origem da captura.");
    expectContains(json, "\"lane_found\":true",
                   "Payload deve refletir lane_found.");
    expectContains(json, "\"confidence_score\":0.870000",
                   "Payload deve serializar confidence_score.");
    expectContains(json, "\"heading_valid\":true",
                   "Payload deve serializar heading_valid.");
    expectContains(json, "\"curvature_valid\":true",
                   "Payload deve serializar curvature_valid.");
    expectContains(json, "\"references\":{\"near\":",
                   "Payload deve conter referencias.");
    expectContains(json, "\"sample_count\":12",
                   "Payload deve carregar sample_count das referencias.");
}

TestRegistrar telemetry_test("road_segmentation_telemetry_serialization",
                             testTelemetrySerialization);

} // namespace
