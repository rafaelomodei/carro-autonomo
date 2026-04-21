#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignDebugRenderer.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace ts = autonomous_car::services::traffic_sign_detection;
namespace vision = autonomous_car::services::vision;

vision::VisionRuntimeTelemetry makeRuntimeTelemetry() {
    vision::VisionRuntimeTelemetry telemetry;
    telemetry.traffic_sign_fps = 3.8;
    telemetry.traffic_sign_inference_ms = 12.4;
    telemetry.sign_result_age_ms = 140;
    return telemetry;
}

ts::TrafficSignFrameResult makeDebugResult() {
    ts::TrafficSignFrameResult result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Confirmed,
        ts::buildTrafficSignRoi({320, 240}, 0.55, 1.0, 0.08, 0.72, true),
        123456);
    result.model_labels_summary = "Parada Obrigatoria sign, Vire a esquerda sign";
    result.debug_roi_frame = cv::Mat(154, 144, CV_8UC3, cv::Scalar{35, 35, 35});
    result.debug_model_input_frame = cv::Mat(196, 196, CV_8UC1, cv::Scalar{180});

    ts::TrafficSignDetection detection;
    detection.sign_id = ts::TrafficSignId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = "Parada obrigatoria";
    detection.confidence_score = 0.94;
    detection.bbox_roi = {30, 24, 48, 46};
    detection.bbox_frame = {206, 43, 48, 46};
    detection.consecutive_frames = 3;
    detection.required_frames = 2;
    detection.confirmed_at_ms = 123400;
    detection.last_seen_at_ms = 123456;
    result.raw_detections.push_back(detection);
    result.active_detection = detection;
    return result;
}

double imageDifference(const cv::Mat &lhs, const cv::Mat &rhs) {
    cv::Mat diff;
    cv::absdiff(lhs, rhs, diff);
    const cv::Scalar diff_sum = cv::sum(diff);
    return diff_sum[0] + diff_sum[1] + diff_sum[2];
}

void testRendererProducesDedicatedDebugPanelWithoutFrames() {
    ts::TrafficSignDebugRenderer renderer;
    const cv::Mat panel =
        renderer.render(ts::TrafficSignFrameResult{}, makeRuntimeTelemetry());

    expect(!panel.empty(), "Renderer dedicado deve produzir painel mesmo sem frames de debug.");
    expect(panel.cols > 0 && panel.rows > 0,
           "Painel dedicado deve ter dimensoes validas.");
}

void testRendererShowsPreprocessedModelInput() {
    ts::TrafficSignDebugRenderer renderer;
    ts::TrafficSignFrameResult without_input = makeDebugResult();
    without_input.debug_model_input_frame.release();

    const cv::Mat without_model_input =
        renderer.render(without_input, makeRuntimeTelemetry());
    const cv::Mat with_model_input =
        renderer.render(makeDebugResult(), makeRuntimeTelemetry());

    expect(imageDifference(without_model_input, with_model_input) > 0.0,
           "Painel deve mudar quando o input preprocessado do modelo estiver presente.");
}

void testRendererDrawsTrafficSignBoxesOnRoiTile() {
    ts::TrafficSignDebugRenderer renderer;
    ts::TrafficSignFrameResult without_detection = makeDebugResult();
    without_detection.raw_detections.clear();
    without_detection.active_detection.reset();
    without_detection.detector_state = ts::TrafficSignDetectorState::Idle;

    const cv::Mat without_overlay =
        renderer.render(without_detection, makeRuntimeTelemetry());
    const cv::Mat with_overlay =
        renderer.render(makeDebugResult(), makeRuntimeTelemetry());

    expect(imageDifference(without_overlay, with_overlay) > 0.0,
           "Painel deve desenhar boxes de deteccao na ROI dedicada.");
}

void testRendererHighlightsErrorState() {
    ts::TrafficSignDebugRenderer renderer;
    ts::TrafficSignFrameResult idle = makeDebugResult();
    idle.detector_state = ts::TrafficSignDetectorState::Idle;
    idle.raw_detections.clear();
    idle.active_detection.reset();
    idle.last_error.clear();

    ts::TrafficSignFrameResult error = idle;
    error.detector_state = ts::TrafficSignDetectorState::Error;
    error.last_error = "Modelo Edge Impulse incompativel";

    const cv::Mat idle_panel = renderer.render(idle, makeRuntimeTelemetry());
    const cv::Mat error_panel = renderer.render(error, makeRuntimeTelemetry());

    expect(imageDifference(idle_panel, error_panel) > 0.0,
           "Painel deve destacar visualmente o estado de erro.");
}

TestRegistrar traffic_sign_debug_renderer_placeholder_test(
    "traffic_sign_debug_renderer_produces_panel_without_frames",
    testRendererProducesDedicatedDebugPanelWithoutFrames);
TestRegistrar traffic_sign_debug_renderer_model_input_test(
    "traffic_sign_debug_renderer_shows_preprocessed_model_input",
    testRendererShowsPreprocessedModelInput);
TestRegistrar traffic_sign_debug_renderer_boxes_test(
    "traffic_sign_debug_renderer_draws_roi_boxes",
    testRendererDrawsTrafficSignBoxesOnRoiTile);
TestRegistrar traffic_sign_debug_renderer_error_test(
    "traffic_sign_debug_renderer_highlights_error_state",
    testRendererHighlightsErrorState);

} // namespace
