#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignRuntime.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace ts = autonomous_car::services::traffic_sign_detection;

ts::TrafficSignFrameResult makeConfirmedResult(std::int64_t timestamp_ms) {
    ts::TrafficSignFrameResult result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Confirmed,
        ts::buildTrafficSignRoi({320, 240}, 0.45, 0.08, 0.72), timestamp_ms);

    ts::TrafficSignDetection detection;
    detection.sign_id = ts::TrafficSignId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = "Parada obrigatoria";
    detection.confidence_score = 0.96;
    detection.bbox_frame = {220, 50, 48, 48};
    detection.bbox_roi = {75, 20, 48, 48};
    detection.consecutive_frames = 2;
    detection.required_frames = 2;
    detection.confirmed_at_ms = timestamp_ms - 20;
    detection.last_seen_at_ms = timestamp_ms;
    result.raw_detections.push_back(detection);
    result.active_detection = detection;
    return result;
}

void testTrafficSignRuntimeKeepsFreshDetectionsVisible() {
    const auto fresh_result = makeConfirmedResult(1'000);
    const auto renderable = ts::buildRenderableTrafficSignResult(fresh_result, 1'300, 500);

    expect(renderable.active_detection.has_value(),
           "Deteccao fresca deve continuar visivel no overlay.");
    expect(renderable.detector_state == ts::TrafficSignDetectorState::Confirmed,
           "Estado confirmado deve ser preservado enquanto o resultado estiver fresco.");
}

void testTrafficSignRuntimeHidesExpiredDetectionsFromOverlay() {
    const auto stale_result = makeConfirmedResult(1'000);
    const auto renderable = ts::buildRenderableTrafficSignResult(stale_result, 1'600, 500);

    expect(!renderable.active_detection.has_value(),
           "Deteccao expirada nao deve continuar aparecendo no overlay.");
    expect(renderable.raw_detections.empty(),
           "Deteccoes brutas expirada devem ser removidas do resultado renderizavel.");
    expect(renderable.detector_state == ts::TrafficSignDetectorState::Idle,
           "Resultado stale deve voltar ao estado idle para a visualizacao.");
}

TestRegistrar traffic_sign_runtime_fresh_test(
    "traffic_sign_runtime_keeps_fresh_detections_visible",
    testTrafficSignRuntimeKeepsFreshDetectionsVisible);
TestRegistrar traffic_sign_runtime_stale_test(
    "traffic_sign_runtime_hides_expired_detections_from_overlay",
    testTrafficSignRuntimeHidesExpiredDetectionsFromOverlay);

} // namespace
