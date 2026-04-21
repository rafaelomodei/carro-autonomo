#include "TestRegistry.hpp"
#include "policy/TrafficSignTemporalFilter.hpp"

namespace {

using traffic_sign_service::SignalDetection;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::TrafficSignDetectorState;
using traffic_sign_service::TrafficSignFrameResult;
using traffic_sign_service::buildTrafficSignRoi;
using traffic_sign_service::makeTrafficSignFrameResult;
using traffic_sign_service::policy::TrafficSignTemporalFilter;
using traffic_sign_service::policy::TrafficSignTemporalFilterConfig;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

SignalDetection makeDetection(std::int64_t timestamp_ms) {
    SignalDetection detection;
    detection.signal_id = TrafficSignalId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = traffic_sign_service::displayLabel(TrafficSignalId::Stop);
    detection.confidence = 0.92;
    detection.bbox_frame = {100, 50, 40, 40};
    detection.bbox_roi = {10, 12, 40, 40};
    detection.last_seen_at_ms = timestamp_ms;
    return detection;
}

TrafficSignFrameResult makeFrameResult(std::int64_t timestamp_ms, bool with_detection) {
    TrafficSignFrameResult frame_result = makeTrafficSignFrameResult(
        TrafficSignDetectorState::Idle,
        buildTrafficSignRoi({320, 240}, 0.55, 1.0, 0.08, 0.72), timestamp_ms);
    if (with_detection) {
        frame_result.raw_detections.push_back(makeDetection(timestamp_ms));
    }
    return frame_result;
}

void testTemporalFilterTransitionsCandidateToConfirmed() {
    TrafficSignTemporalFilter filter(TrafficSignTemporalFilterConfig{0.60, 2, 3});

    auto first = filter.update(makeFrameResult(1000, true));
    expect(first.detector_state == TrafficSignDetectorState::Candidate,
           "Primeiro frame deve virar candidate.");

    auto second = filter.update(makeFrameResult(1100, true));
    expect(second.detector_state == TrafficSignDetectorState::Confirmed,
           "Segundo frame consecutivo deve confirmar a placa.");
    expect(second.active_detection.has_value(),
           "Resultado confirmado deve preencher active_detection.");
}

void testTemporalFilterKeepsConfirmedStateForMissedFramesThenResets() {
    TrafficSignTemporalFilter filter(TrafficSignTemporalFilterConfig{0.60, 2, 1});

    filter.update(makeFrameResult(1000, true));
    filter.update(makeFrameResult(1100, true));

    auto kept = filter.update(makeFrameResult(1200, false));
    expect(kept.detector_state == TrafficSignDetectorState::Confirmed,
           "Primeiro frame perdido deve manter confirmado dentro da tolerancia.");

    auto reset = filter.update(makeFrameResult(1300, false));
    expect(reset.detector_state == TrafficSignDetectorState::Idle,
           "Ao exceder max_missed_frames o estado deve voltar para idle.");
}

TestRegistrar temporal_filter_confirm_test(
    "traffic_sign_temporal_filter_transitions_candidate_to_confirmed",
    testTemporalFilterTransitionsCandidateToConfirmed);
TestRegistrar temporal_filter_missed_test(
    "traffic_sign_temporal_filter_keeps_confirmed_then_resets_after_misses",
    testTemporalFilterKeepsConfirmedStateForMissedFramesThenResets);

} // namespace
