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

ts::TrafficSignFrameResult makeTrafficSignResult() {
    ts::TrafficSignFrameResult result = ts::makeTrafficSignFrameResult(
        ts::TrafficSignDetectorState::Confirmed,
        ts::buildTrafficSignRoi({320, 240}, 0.45, 0.08, 0.72), 123456);

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

void testDashboardReceivesTrafficSignStatus() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat dashboard = renderer.render(
        vision::VisionDebugViewId::Dashboard, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry, makeTrafficSignResult());

    expect(!dashboard.empty(), "Dashboard deve ser renderizado com sucesso.");
}

TestRegistrar vision_annotated_overlay_test("vision_debug_view_renderer_annotated_overlay",
                                            testAnnotatedViewReceivesTrafficSignOverlay);
TestRegistrar vision_dashboard_overlay_test("vision_debug_view_renderer_dashboard_overlay",
                                            testDashboardReceivesTrafficSignStatus);

} // namespace
