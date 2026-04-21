#include <cstdint>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignTemporalFilter.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace ts = autonomous_car::services::traffic_sign_detection;

ts::TrafficSignDetection makeDetection(std::int64_t timestamp_ms) {
    ts::TrafficSignDetection detection;
    detection.sign_id = ts::TrafficSignId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = "Parada obrigatoria";
    detection.confidence_score = 0.95;
    detection.bbox_frame = {120, 50, 40, 40};
    detection.bbox_roi = {20, 10, 40, 40};
    detection.required_frames = 2;
    detection.last_seen_at_ms = timestamp_ms;
    return detection;
}

ts::TrafficSignFrameResult makeFrameResult(std::int64_t timestamp_ms, bool with_detection) {
    ts::TrafficSignFrameResult frame_result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Idle,
        ts::buildTrafficSignRoi({320, 240}, 0.45, 0.08, 0.72), timestamp_ms);

    if (with_detection) {
        frame_result.raw_detections.push_back(makeDetection(timestamp_ms));
    }

    return frame_result;
}

void testTemporalFilterCandidateAndConfirmation() {
    ts::TrafficSignConfig config;
    config.min_consecutive_frames = 2;
    config.max_missed_frames = 1;

    ts::TrafficSignTemporalFilter filter(config);

    const auto first = filter.update(makeFrameResult(1000, true));
    expect(first.detector_state == ts::TrafficSignDetectorState::Candidate,
           "Primeira deteccao deve entrar em estado candidate.");
    expect(first.candidate.has_value() && first.candidate->consecutive_frames == 1,
           "Candidate deve iniciar com um frame consecutivo.");

    const auto second = filter.update(makeFrameResult(1010, true));
    expect(second.detector_state == ts::TrafficSignDetectorState::Confirmed,
           "Segunda deteccao consecutiva deve confirmar.");
    expect(second.active_detection.has_value() &&
               second.active_detection->consecutive_frames >= 2,
           "Deteccao confirmada deve carregar contagem acumulada.");
}

void testTemporalFilterDropsDetectionAfterMissedFrames() {
    ts::TrafficSignConfig config;
    config.min_consecutive_frames = 1;
    config.max_missed_frames = 1;

    ts::TrafficSignTemporalFilter filter(config);

    const auto confirmed = filter.update(makeFrameResult(2000, true));
    expect(confirmed.detector_state == ts::TrafficSignDetectorState::Confirmed,
           "Min consecutive frames igual a um deve confirmar imediatamente.");

    const auto first_miss = filter.update(makeFrameResult(2010, false));
    expect(first_miss.detector_state == ts::TrafficSignDetectorState::Confirmed,
           "Primeiro frame perdido ainda deve segurar a deteccao confirmada.");

    const auto second_miss = filter.update(makeFrameResult(2020, false));
    expect(second_miss.detector_state == ts::TrafficSignDetectorState::Idle,
           "Ao exceder max_missed_frames o estado deve voltar para idle.");
    expect(!second_miss.active_detection.has_value(),
           "Deteccao ativa deve ser limpa apos exceder frames perdidos.");
}

TestRegistrar traffic_sign_filter_confirm_test(
    "traffic_sign_temporal_filter_candidate_and_confirmation",
    testTemporalFilterCandidateAndConfirmation);
TestRegistrar traffic_sign_filter_miss_test(
    "traffic_sign_temporal_filter_drops_detection_after_missed_frames",
    testTemporalFilterDropsDetectionAfterMissedFrames);

} // namespace
