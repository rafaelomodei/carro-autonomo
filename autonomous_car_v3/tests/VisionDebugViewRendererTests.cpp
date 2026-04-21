#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/autonomous_control/AutonomousControlTypes.hpp"
#include "services/vision/VisionDebugViewRenderer.hpp"
#include "services/vision/VisionRuntimeTelemetry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace autoctrl = autonomous_car::services::autonomous_control;
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

void testAnnotatedViewRendersWithoutTrafficSignOverlay() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    vision::VisionRuntimeTelemetry runtime_telemetry;

    const cv::Mat annotated = renderer.render(
        vision::VisionDebugViewId::Annotated, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry);

    expect(!annotated.empty(), "Annotated view deve ser renderizada.");
    expect(annotated.size() == segmentation_result.resized_frame.size(),
           "Annotated view deve acompanhar o tamanho do frame principal.");
}

void testDashboardRendersWithAutonomousControlPanel() {
    vision::VisionDebugViewRenderer renderer;
    const auto segmentation_result = makeSegmentationResult();
    rsl::config::LabConfig config;
    autoctrl::AutonomousControlSnapshot snapshot;
    snapshot.driving_mode = autonomous_car::DrivingMode::Autonomous;
    snapshot.autonomous_started = true;
    snapshot.tracking_state = autoctrl::TrackingState::Tracking;
    snapshot.motion_command = autoctrl::MotionCommand::Forward;
    snapshot.lane_available = true;
    snapshot.confidence_ok = true;
    snapshot.confidence_score = 0.82;
    snapshot.preview_error = 0.05;
    snapshot.steering_command = 0.08;
    snapshot.projected_path = {{0.0, 0.0}, {0.05, 0.3}, {0.08, 0.6}, {0.10, 1.0}};

    vision::VisionRuntimeTelemetry runtime_telemetry;
    runtime_telemetry.core_fps = 20.0;
    runtime_telemetry.stream_fps = 5.0;
    runtime_telemetry.stream_encode_ms = 18.0;

    const cv::Mat dashboard = renderer.render(
        vision::VisionDebugViewId::Dashboard, segmentation_result, config, "camera",
        "calibrated", snapshot, runtime_telemetry);

    expect(!dashboard.empty(), "Dashboard deve ser renderizado com sucesso.");
    expect(dashboard.cols > segmentation_result.resized_frame.cols,
           "Dashboard deve anexar o painel lateral de controle.");
}

TestRegistrar vision_annotated_test(
    "vision_debug_view_renderer_renders_annotated_view_without_local_traffic_sign_overlay",
    testAnnotatedViewRendersWithoutTrafficSignOverlay);
TestRegistrar vision_dashboard_test(
    "vision_debug_view_renderer_renders_dashboard_with_control_panel",
    testDashboardRendersWithAutonomousControlPanel);

} // namespace
