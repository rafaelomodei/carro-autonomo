#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationPipeline.hpp"

namespace {

using road_segmentation_lab::config::LabConfig;
using road_segmentation_lab::config::SegmentationMode;
using road_segmentation_lab::pipeline::RoadSegmentationPipeline;
using road_segmentation_lab::pipeline::RoadSegmentationResult;

void expect(bool condition, const std::string &message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expectNear(double actual, double expected, double tolerance, const std::string &message) {
    if (std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(message + " (actual=" + std::to_string(actual) +
                                 ", expected=" + std::to_string(expected) + ")");
    }
}

LabConfig makeBaseConfig() {
    LabConfig config;
    config.resize_enabled = false;
    config.roi_top_ratio = 0.5;
    config.roi_bottom_ratio = 1.0;
    config.gaussian_enabled = false;
    config.clahe_enabled = false;
    config.morph_enabled = true;
    config.morph_kernel = cv::Size(5, 5);
    config.morph_iterations = 1;
    config.min_contour_area = 200.0;
    config.gray_threshold = 90;
    config.adaptive_block_size = 31;
    config.adaptive_c = 9.0;
    return config;
}

cv::Mat makeLaneFrame(int width, int height, int bottom_center_x, int top_center_x, int lane_width,
                      bool add_noise = false) {
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

    const int roi_top = height / 2;
    const int half_width = lane_width / 2;
    std::vector<cv::Point> polygon = {
        {top_center_x - half_width, roi_top},
        {top_center_x + half_width, roi_top},
        {bottom_center_x + half_width, height - 1},
        {bottom_center_x - half_width, height - 1},
    };

    cv::fillConvexPoly(image, polygon, cv::Scalar(0, 0, 0), cv::LINE_AA);

    if (add_noise) {
        cv::rectangle(image, {5, height - 55}, {25, height - 25}, cv::Scalar(0, 0, 0), cv::FILLED);
    }

    return image;
}

cv::Mat addVehicleHood(const cv::Mat &frame) {
    cv::Mat image = frame.clone();
    const cv::Point center(image.cols / 2, image.rows - 18);
    const cv::Size axes(60, 26);
    cv::ellipse(image, center, axes, 0, 0, 360, cv::Scalar(0, 0, 255), cv::FILLED, cv::LINE_AA);
    return image;
}

RoadSegmentationResult runPipeline(const LabConfig &config, const cv::Mat &frame) {
    RoadSegmentationPipeline pipeline(config);
    return pipeline.process(frame);
}

void testCenteredLane() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 160, 160, 120);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Centered lane should be detected.");
    expectNear(result.lane_center_ratio, 0.5, 0.05, "Centered lane ratio should stay near 0.5.");
    expectNear(result.steering_error_normalized, 0.0, 0.1,
               "Centered lane should have near-zero normalized error.");
    expect(result.left_boundary_points.size() >= 6 && result.right_boundary_points.size() >= 6,
           "Both lane boundaries should expose real boundary polylines.");
    expect(result.road_polygon_points.size() >= 12,
           "Centered lane should produce a filled road polygon.");
}

void testShiftedLeftLane() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 120, 135, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Left-shifted lane should be detected.");
    expect(result.lane_center_ratio < 0.5, "Left-shifted lane ratio should be below 0.5.");
    expect(result.steering_error_normalized < 0.0,
           "Left-shifted lane should produce a negative normalized error.");
}

void testShiftedRightLane() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 205, 190, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Right-shifted lane should be detected.");
    expect(result.lane_center_ratio > 0.5, "Right-shifted lane ratio should be above 0.5.");
    expect(result.steering_error_normalized > 0.0,
           "Right-shifted lane should produce a positive normalized error.");
}

void testCurvedLaneLikeShape() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 200, 130, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Curved/slanted lane should still be detected.");
    expect(result.lane_center_ratio > 0.5,
           "Curved lane ending on the right should bias the center ratio to the right.");
}

void testNoiseOutsideMainRoad() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 160, 160, 120, true);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Small dark noise should not suppress the main road detection.");
    expectNear(result.lane_center_ratio, 0.5, 0.06,
               "Noise should not significantly move the detected road center.");
}

void testNoLaneScenario() {
    LabConfig config = makeBaseConfig();
    cv::Mat frame(320, 320, CV_8UC3, cv::Scalar(255, 255, 255));
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(!result.lane_found, "Blank frame should not report lane_found.");
}

void testResizeAndRoi() {
    LabConfig config = makeBaseConfig();
    config.resize_enabled = true;
    config.target_width = 320;
    config.target_height = 240;

    const cv::Mat frame = makeLaneFrame(640, 480, 320, 320, 240);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.resized_frame.size() == cv::Size(320, 240), "Resize stage should output 320x240.");
    expect(result.roi_frame.size() == cv::Size(320, 120), "ROI stage should keep the lower half.");
    expect(result.roi_rect == cv::Rect(0, 120, 320, 120), "ROI rect should match the expected crop.");
    expect(result.roi_polygon_points.size() == 4, "ROI trapezoid should expose four polygon points.");
}

void testAllSegmentationModes() {
    const cv::Mat frame = makeLaneFrame(320, 240, 160, 160, 120);
    const std::vector<SegmentationMode> modes = {
        SegmentationMode::HsvDark,
        SegmentationMode::GrayThreshold,
        SegmentationMode::LabDark,
        SegmentationMode::AdaptiveGray,
    };

    for (SegmentationMode mode : modes) {
        LabConfig config = makeBaseConfig();
        config.segmentation_mode = mode;
        const RoadSegmentationResult result = runPipeline(config, frame);

        expect(result.lane_found,
               "Every segmentation mode should detect the synthetic centered lane.");
    }
}

void testHoodMaskRemovesVehicleInfluence() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = addVehicleHood(makeLaneFrame(320, 240, 160, 160, 110));

    LabConfig without_hood_mask = config;
    without_hood_mask.hood_mask_enabled = false;

    const RoadSegmentationResult masked_result = runPipeline(config, frame);
    const RoadSegmentationResult unmasked_result = runPipeline(without_hood_mask, frame);

    expect(masked_result.hood_mask_polygon_points.size() > 8,
           "Enabled hood mask should expose its exclusion polygon.");
    expect(masked_result.lane_found, "Lane should remain detectable with hood masking.");
    expectNear(masked_result.lane_center_ratio, 0.5, 0.08,
               "Hood masking should keep the center near the road midpoint.");
    expect(masked_result.mask_area_px < unmasked_result.mask_area_px,
           "Hood masking should reduce the segmented area contaminated by the vehicle hood.");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"centered_lane", testCenteredLane},
        {"shifted_left_lane", testShiftedLeftLane},
        {"shifted_right_lane", testShiftedRightLane},
        {"curved_lane_like_shape", testCurvedLaneLikeShape},
        {"noise_outside_main_road", testNoiseOutsideMainRoad},
        {"no_lane_scenario", testNoLaneScenario},
        {"resize_and_roi", testResizeAndRoi},
        {"all_segmentation_modes", testAllSegmentationModes},
        {"hood_mask_removes_vehicle_influence", testHoodMaskRemovesVehicleInfluence},
    };

    for (const auto &[name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception &ex) {
            std::cerr << "[FAIL] " << name << ": " << ex.what() << '\n';
            return 1;
        }
    }

    return 0;
}
