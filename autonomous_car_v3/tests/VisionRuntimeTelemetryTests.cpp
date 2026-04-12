#include "TestRegistry.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expectContains;
namespace vision = autonomous_car::services::vision;

void testVisionRuntimeTelemetryJsonContainsRuntimeFields() {
    vision::VisionRuntimeTelemetry telemetry;
    telemetry.timestamp_ms = 123456789;
    telemetry.source = "Camera index 0";
    telemetry.core_fps = 18.4;
    telemetry.stream_fps = 4.2;
    telemetry.traffic_sign_fps = 3.9;
    telemetry.traffic_sign_inference_ms = 11.7;
    telemetry.stream_encode_ms = 28.3;
    telemetry.traffic_sign_dropped_frames = 5;
    telemetry.stream_dropped_frames = 8;
    telemetry.sign_result_age_ms = 120;

    const std::string json = vision::buildVisionRuntimeTelemetryJson(telemetry);

    expectContains(json, "\"type\":\"telemetry.vision_runtime\"",
                   "JSON deve expor o tipo da telemetria de runtime.");
    expectContains(json, "\"core_fps\":18.400000",
                   "JSON deve expor core_fps com precisao fixa.");
    expectContains(json, "\"stream_fps\":4.200000",
                   "JSON deve expor stream_fps com precisao fixa.");
    expectContains(json, "\"traffic_sign_dropped_frames\":5",
                   "JSON deve expor descarte de frames da sinalizacao.");
    expectContains(json, "\"stream_dropped_frames\":8",
                   "JSON deve expor descarte de frames do stream.");
    expectContains(json, "\"sign_result_age_ms\":120",
                   "JSON deve expor a idade do ultimo resultado de sinalizacao.");
}

TestRegistrar vision_runtime_json_test(
    "vision_runtime_telemetry_json_contains_runtime_fields",
    testVisionRuntimeTelemetryJsonContainsRuntimeFields);

} // namespace
