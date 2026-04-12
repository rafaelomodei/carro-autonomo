#include <string>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignTelemetry.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expectContains;
namespace ts = autonomous_car::services::traffic_sign_detection;

void testTrafficSignTelemetrySerialization() {
    ts::TrafficSignFrameResult frame_result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Confirmed,
        ts::buildTrafficSignRoi({320, 240}, 0.55, 1.0, 0.08, 0.72), 123456);

    ts::TrafficSignDetection detection;
    detection.sign_id = ts::TrafficSignId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = "Parada obrigatoria";
    detection.confidence_score = 0.93;
    detection.bbox_frame = {100, 50, 40, 40};
    detection.bbox_roi = {10, 12, 40, 40};
    detection.consecutive_frames = 2;
    detection.required_frames = 2;
    detection.confirmed_at_ms = 123450;
    detection.last_seen_at_ms = 123456;

    frame_result.raw_detections.push_back(detection);
    frame_result.active_detection = detection;

    const std::string json =
        ts::buildTrafficSignTelemetryJson(frame_result, "Camera index 0");

    expectContains(json, "\"type\":\"telemetry.traffic_sign_detection\"",
                   "Payload deve identificar o tipo da telemetria.");
    expectContains(json, "\"detector_state\":\"confirmed\"",
                   "Payload deve serializar detector_state.");
    expectContains(json, "\"source\":\"Camera index 0\"",
                   "Payload deve serializar source.");
    expectContains(json, "\"sign_id\":\"stop\"",
                   "Payload deve serializar o sign_id.");
    expectContains(json, "\"left_ratio\":0.550000",
                   "Payload deve serializar o left_ratio.");
    expectContains(json, "\"right_ratio\":1.000000",
                   "Payload deve serializar o right_ratio.");
    expectContains(json, "\"right_width_ratio\":0.450000",
                   "Payload deve manter a largura derivada para compatibilidade.");
    expectContains(json, "\"active_detection\":{",
                   "Payload deve conter active_detection.");
    expectContains(json, "\"raw_detections\":[{",
                   "Payload deve conter raw detections.");
}

TestRegistrar traffic_sign_telemetry_test("traffic_sign_telemetry_serialization",
                                          testTrafficSignTelemetrySerialization);

} // namespace
