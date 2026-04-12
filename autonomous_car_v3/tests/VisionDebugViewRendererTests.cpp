#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "TestRegistry.hpp"
#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/autonomous_control/AutonomousControlTypes.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"
#include "services/vision/VisionDebugViewRenderer.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace autoctrl = autonomous_car::services::autonomous_control;
namespace ts = autonomous_car::services::traffic_sign_detection;
namespace vision = autonomous_car::services::vision;
namespace rsl = road_segmentation_lab;

rsl::pipeline::RoadSegmentationResult makeSegmentationResult() {
    rsl::pipeline::RoadSegmentationResult result;
    result.resized_frame = cv::Mat(240, 320, CV_8UC3, cv::Scalar{40, 40, 40});
    result.preprocessed_frame = cv::Mat(120, 320, CV_8UC1, cv::Scalar{90});
    result.mask_frame = cv::Mat(120, 320, CV_8UC1, cv::Scalar{140});
    result.roi_rect = cv::Rect(0, 120, 320, 120);
    result.frame_center = cv::Point(160, 120);
    result.lane_center = cv::Point(170, 180);
    result.lane_found = true;
    result.confidence_score = 0.82;
    result.segmentation_mode = "GRAY_THRESHOLD";
    result.left_boundary_points = {{100, 239}, {130, 160}, {145, 120}};
    result.right_boundary_points = {{220, 239}, {190, 160}, {175, 120}};
    result.centerline_points = {{160, 239}, {160, 180}, {160, 120}};
    result.road_polygon_points = {{100, 239}, {220, 239}, {175, 120}, {145, 120}};
    result.near_reference = {{162, 205}, 190, 220, 0.51, 4.0, 0.02, 12, true};
    result.mid_reference = {{160, 170}, 150, 180, 0.50, 0.0, 0.00, 12, true};
    result.far_reference = {{158, 135}, 120, 150, 0.49, -3.0, -0.02, 12, true};
    return result;
}

ts::TrafficSignFrameResult makeTrafficSignResult(bool debug_roi_enabled = true,
                                                 cv::Size source_frame_size = {320, 240}) {
    ts::TrafficSignFrameResult result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Confirmed,
        ts::buildTrafficSignRoi(source_frame_size, 0.55, 1.0, 0.08, 0.72,
                                debug_roi_enabled),
        123456);

    ts::TrafficSignDetection detection;
    detection.sign_id = ts::TrafficSignId::Stop;
    detection.model_label = "Parada Obrigatoria sign";
    detection.display_label = "Parada obrigatoria";
    detection.confidence_score = 0.94;
    detection.bbox_frame = {220, 50, 50, 50};
    detection.bbox_roi = {76, 31, 50, 50};
    detection.consecutive_frames = 3;
    detection.required_frames = 2;
    detection.confirmed_at_ms = 123450;
    detection.last_seen_at_ms = 123456;
    result.raw_detections.push_back(detection);
    result.active_detection = detection;
    return result;
}

ts::TrafficSignFrameResult makeTrafficSignRoiOnlyResult(
    bool debug_roi_enabled = true, cv::Size source_frame_size = {320, 240}) {
    return ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Idle,
        ts::buildTrafficSignRoi(source_frame_size, 0.55, 1.0, 0.08, 0.72,
                                debug_roi_enabled),
        123456);
}

void testAnnotatedViewReceivesTrafficSignOverlay() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat without_overlay = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, ts::TrafficSignFrameResult{});
    const cv::Mat with_overlay = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, makeTrafficSignResult());

    cv::Mat diff;
    cv::absdiff(without_overlay, with_overlay, diff);
    const cv::Scalar diff_sum = cv::sum(diff);
    const double total_diff = diff_sum[0] + diff_sum[1] + diff_sum[2];

    expect(total_diff > 0.0,
           "Overlay de sinalizacao deve alterar a annotated view.");
}

void testAnnotatedViewCanHideOnlyTrafficSignRoiOutline() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat without_overlay = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, ts::TrafficSignFrameResult{});
    const cv::Mat detection_only = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, makeTrafficSignResult(false));
    const cv::Mat with_roi = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, makeTrafficSignResult(true));

    cv::Mat diff_detection_only;
    cv::absdiff(without_overlay, detection_only, diff_detection_only);
    const cv::Scalar detection_only_sum = cv::sum(diff_detection_only);
    const double total_detection_only_diff =
        detection_only_sum[0] + detection_only_sum[1] + detection_only_sum[2];

    expect(total_detection_only_diff > 0.0,
           "Boxes de deteccao devem continuar visiveis sem o contorno da ROI.");
    expect(detection_only.at<cv::Vec3b>(75, 220) == with_roi.at<cv::Vec3b>(75, 220),
           "Desligar a ROI nao deve alterar o box da deteccao.");
    expect(detection_only.at<cv::Vec3b>(19, 176) != with_roi.at<cv::Vec3b>(19, 176),
           "Contorno da ROI deve desaparecer quando a flag estiver desligada.");
}

void testAnnotatedViewScalesTrafficSignOverlayToRenderedFrame() {
    vision::VisionDebugViewRenderer renderer;
    auto segmentation_result = makeSegmentationResult();
    segmentation_result.resized_frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar{40, 40, 40});
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat without_overlay = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, ts::TrafficSignFrameResult{});
    const cv::Mat with_roi_only = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry,
        makeTrafficSignRoiOnlyResult(true, {320, 240}));
    const cv::Mat with_overlay = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry,
        makeTrafficSignResult(true, {320, 240}));

    cv::Mat box_diff;
    cv::absdiff(with_roi_only, with_overlay, box_diff);
    const cv::Scalar box_probe_sum = cv::sum(box_diff(cv::Rect(438, 98, 8, 8)));

    expect(box_probe_sum[0] + box_probe_sum[1] + box_probe_sum[2] > 0.0,
           "Boxes de deteccao devem acompanhar a escala da annotated view.");
}

void testDashboardReceivesTrafficSignStatus() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat dashboard_without_overlay = renderer.render(
        vision::VisionDebugViewId::Dashboard, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, ts::TrafficSignFrameResult{});
    const cv::Mat dashboard_with_roi_only = renderer.render(
        vision::VisionDebugViewId::Dashboard, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry,
        makeTrafficSignRoiOnlyResult(true));
    const cv::Mat dashboard = renderer.render(
        vision::VisionDebugViewId::Dashboard, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, makeTrafficSignResult());

    expect(!dashboard.empty(), "Dashboard deve ser renderizado com sucesso.");
    cv::Mat roi_diff;
    cv::absdiff(dashboard_without_overlay, dashboard_with_roi_only, roi_diff);
    const int tile_width = segmentation_result.resized_frame.cols;
    const int tile_height = segmentation_result.resized_frame.rows;
    const int tile_card_width = tile_width + 250;
    const cv::Scalar original_roi_sum =
        cv::sum(roi_diff(cv::Rect(0, 0, tile_width, tile_height)));
    const cv::Scalar annotated_roi_sum = cv::sum(
        roi_diff(cv::Rect(tile_card_width, tile_height, tile_width, tile_height)));

    expect(original_roi_sum[0] + original_roi_sum[1] + original_roi_sum[2] > 0.0,
           "Dashboard local deve desenhar a ROI de sinalizacao no tile original.");
    expect(annotated_roi_sum[0] + annotated_roi_sum[1] + annotated_roi_sum[2] > 0.0,
           "Dashboard local deve desenhar a ROI de sinalizacao no tile anotado.");
}

TestRegistrar vision_annotated_overlay_test("vision_debug_view_renderer_annotated_overlay",
                                            testAnnotatedViewReceivesTrafficSignOverlay);
TestRegistrar vision_annotated_roi_toggle_test(
    "vision_debug_view_renderer_hides_only_traffic_sign_roi_outline",
    testAnnotatedViewCanHideOnlyTrafficSignRoiOutline);
TestRegistrar vision_annotated_resize_test(
    "vision_debug_view_renderer_scales_traffic_sign_overlay_to_rendered_frame",
    testAnnotatedViewScalesTrafficSignOverlayToRenderedFrame);
TestRegistrar vision_dashboard_overlay_test("vision_debug_view_renderer_dashboard_overlay",
                                            testDashboardReceivesTrafficSignStatus);

} // namespace
