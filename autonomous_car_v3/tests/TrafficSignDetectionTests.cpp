#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignTelemetry.hpp"
#include "services/traffic_sign_detection/TrafficSignTemporalFilter.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::tests::expectContains;
namespace ts = autonomous_car::services::traffic_sign_detection;

std::filesystem::path writeTempFile(const std::string &name, const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

ts::TrafficSignDetection makeDetection(ts::TrafficSignId sign_id, double confidence,
                                       int x = 10, int y = 20) {
    ts::TrafficSignDetection detection;
    detection.sign_id = sign_id;
    detection.model_label = std::string(ts::toString(sign_id));
    detection.display_label = ts::displayLabel(sign_id);
    detection.confidence_score = confidence;
    detection.bbox_frame = {x, y, 24, 24};
    detection.bbox_roi = {x - 5, y - 5, 24, 24};
    return detection;
}

void testTrafficSignConfigLoadAndRoi() {
    const auto config_path = writeTempFile(
        "traffic_sign_detection.env",
        "TRAFFIC_SIGN_ENABLED=true\n"
        "TRAFFIC_SIGN_ROI_RIGHT_WIDTH_RATIO=0.35\n"
        "TRAFFIC_SIGN_ROI_TOP_RATIO=0.0\n"
        "TRAFFIC_SIGN_ROI_BOTTOM_RATIO=1.0\n"
        "TRAFFIC_SIGN_MIN_CONFIDENCE=0.72\n"
        "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES=4\n"
        "TRAFFIC_SIGN_MAX_MISSED_FRAMES=3\n"
        "TRAFFIC_SIGN_MAX_RAW_DETECTIONS=6\n");

    ts::TrafficSignConfig config;
    std::vector<std::string> warnings;
    const bool loaded =
        ts::loadTrafficSignConfigFromFile(config_path.string(), config, &warnings);

    expect(loaded, "traffic_sign_detection.env deve carregar com sucesso.");
    expect(config.enabled, "TRAFFIC_SIGN_ENABLED deve ser carregado.");
    expect(config.min_confidence == 0.72, "TRAFFIC_SIGN_MIN_CONFIDENCE deve ser carregado.");
    expect(config.min_consecutive_frames == 4,
           "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES deve ser carregado.");
    expect(config.max_missed_frames == 3,
           "TRAFFIC_SIGN_MAX_MISSED_FRAMES deve ser carregado.");
    expect(config.max_raw_detections == 6,
           "TRAFFIC_SIGN_MAX_RAW_DETECTIONS deve ser carregado.");
    expect(warnings.empty(), "Arquivo valido nao deve gerar warnings.");

    const cv::Rect roi = ts::computeTrafficSignRoi({320, 240}, config);
    expect(roi.x == 208 && roi.y == 0 && roi.width == 112 && roi.height == 240,
           "ROI padrao da direita deve respeitar 320x240 -> x=208, y=0, w=112, h=240.");
}

void testTrafficSignModelCompatibilityWarning() {
    const std::string warning = ts::buildModelCompatibilityWarning(
        {"Parada Obrigatoria sign", "Vire a esquerda sign"});

    expectContains(warning, "turn_right",
                   "Export incompleto deve reportar ausencia da classe turn_right.");
    expect(ts::trafficSignIdFromModelLabel("Vire a direita sign").has_value(),
           "Mapeamento de label futura para turn_right deve existir.");
}

void testTrafficSignTemporalFilter() {
    ts::TrafficSignConfig config;
    config.min_consecutive_frames = 3;
    config.max_missed_frames = 2;

    ts::TrafficSignTemporalFilter filter(config);

    ts::TrafficSignDetectionFrame frame;
    frame.enabled = true;
    frame.roi = {208, 0, 112, 240, 0.35, 0.0, 1.0};
    frame.raw_detections = {makeDetection(ts::TrafficSignId::Stop, 0.91)};

    auto snapshot = filter.update(frame, 1000);
    expect(snapshot.detector_state == ts::TrafficSignDetectorState::Candidate,
           "Primeiro frame valido deve criar candidato.");
    expect(snapshot.candidate.has_value() && !snapshot.active_detection.has_value(),
           "Primeiro frame nao deve confirmar a deteccao.");

    snapshot = filter.update(frame, 1010);
    expect(snapshot.detector_state == ts::TrafficSignDetectorState::Candidate,
           "Segundo frame ainda deve manter candidato.");

    snapshot = filter.update(frame, 1020);
    expect(snapshot.detector_state == ts::TrafficSignDetectorState::Confirmed,
           "Terceiro frame consecutivo deve confirmar a placa.");
    expect(snapshot.active_detection.has_value(),
           "Deteccao confirmada deve preencher active_detection.");
    expect(snapshot.active_detection->confirmed_at_ms == 1020,
           "Timestamp de confirmacao deve ser preservado.");

    ts::TrafficSignDetectionFrame empty_frame;
    empty_frame.enabled = true;
    empty_frame.roi = frame.roi;

    snapshot = filter.update(empty_frame, 1030);
    expect(snapshot.active_detection.has_value(),
           "Primeira perda nao deve limpar a deteccao ativa.");

    snapshot = filter.update(empty_frame, 1040);
    expect(!snapshot.active_detection.has_value(),
           "Segunda perda deve limpar a deteccao ativa com max_missed_frames=2.");
    expect(snapshot.detector_state == ts::TrafficSignDetectorState::Idle,
           "Sem erro e sem deteccoes restantes, o estado deve voltar para idle.");

    filter.reset();
    snapshot = filter.update(frame, 2000);
    expect(snapshot.detector_state == ts::TrafficSignDetectorState::Candidate,
           "Frame isolado apos reset volta a ser apenas candidato.");
}

void testTrafficSignTelemetrySerialization() {
    ts::TrafficSignFrameResult result;
    result.detector_state = ts::TrafficSignDetectorState::Confirmed;
    result.roi = {208, 0, 112, 240, 0.35, 0.0, 1.0};
    result.raw_detections = {makeDetection(ts::TrafficSignId::TurnLeft, 0.82)};

    ts::StabilizedTrafficSignDetection active;
    active.sign_id = ts::TrafficSignId::TurnLeft;
    active.model_label = "Vire a esquerda sign";
    active.display_label = ts::displayLabel(ts::TrafficSignId::TurnLeft);
    active.confidence_score = 0.82;
    active.bbox_frame = {220, 40, 32, 32};
    active.bbox_roi = {12, 40, 32, 32};
    active.consecutive_frames = 3;
    active.required_frames = 3;
    active.confirmed_at_ms = 123450;
    active.last_seen_at_ms = 123456;
    result.active_detection = active;

    const std::string json =
        ts::buildTrafficSignTelemetryJson(result, "Camera index 0", 123456789);

    expectContains(json, "\"type\":\"telemetry.traffic_sign_detection\"",
                   "Payload deve identificar o tipo de telemetria.");
    expectContains(json, "\"detector_state\":\"confirmed\"",
                   "Payload deve serializar detector_state.");
    expectContains(json, "\"right_width_ratio\":0.350000",
                   "Payload deve serializar os ratios da ROI.");
    expectContains(json, "\"active_detection\":{",
                   "Payload deve conter active_detection.");
    expectContains(json, "\"confirmed_at_ms\":123450",
                   "Payload deve serializar confirmed_at_ms.");
}

TestRegistrar traffic_sign_config_test("traffic_sign_config_load_and_roi",
                                       testTrafficSignConfigLoadAndRoi);
TestRegistrar traffic_sign_model_warning_test("traffic_sign_model_compatibility_warning",
                                              testTrafficSignModelCompatibilityWarning);
TestRegistrar traffic_sign_filter_test("traffic_sign_temporal_filter",
                                       testTrafficSignTemporalFilter);
TestRegistrar traffic_sign_telemetry_test("traffic_sign_telemetry_serialization",
                                          testTrafficSignTelemetrySerialization);

} // namespace
