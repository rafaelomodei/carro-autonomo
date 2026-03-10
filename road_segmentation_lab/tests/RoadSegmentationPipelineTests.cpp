#include <cmath>
#include <filesystem>
#include <fstream>
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
using road_segmentation_lab::config::loadConfigFromFile;
using road_segmentation_lab::pipeline::LookaheadReference;
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

cv::Mat makePolylineLaneFrame(int width, int height, const std::vector<int> &center_xs,
                              int lane_width) {
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    if (center_xs.size() < 2) {
        return image;
    }

    const int roi_top = height / 2;
    const int half_width = lane_width / 2;
    std::vector<cv::Point> left_boundary;
    std::vector<cv::Point> right_boundary;
    left_boundary.reserve(center_xs.size());
    right_boundary.reserve(center_xs.size());

    for (std::size_t index = 0; index < center_xs.size(); ++index) {
        const double ratio = static_cast<double>(index) / static_cast<double>(center_xs.size() - 1);
        const int y = roi_top + static_cast<int>(
                                    std::lround(ratio * static_cast<double>((height - 1) - roi_top)));
        left_boundary.emplace_back(center_xs[index] - half_width, y);
        right_boundary.emplace_back(center_xs[index] + half_width, y);
    }

    std::vector<cv::Point> polygon = left_boundary;
    for (auto it = right_boundary.rbegin(); it != right_boundary.rend(); ++it) {
        polygon.push_back(*it);
    }
    cv::fillPoly(image, std::vector<std::vector<cv::Point>>{polygon}, cv::Scalar(0, 0, 0), cv::LINE_AA);
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

std::filesystem::path writeTempConfigFile(const std::string &name, const std::string &contents) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create temporary config file.");
    }
    file << contents;
    return path;
}

void expectReferenceValid(const LookaheadReference &reference, const std::string &label) {
    expect(reference.valid, label + " reference should be valid.");
    expect(reference.sample_count >= 2, label + " reference should aggregate at least two samples.");
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
    expectReferenceValid(result.far_reference, "FAR");
    expectReferenceValid(result.mid_reference, "MID");
    expectReferenceValid(result.near_reference, "NEAR");
    expect(result.far_reference.point.y < result.mid_reference.point.y &&
               result.mid_reference.point.y < result.near_reference.point.y,
           "Lookahead references should be ordered from FAR to NEAR.");
    expectNear(result.far_reference.lateral_offset_px, 0.0, 6.0,
               "Centered FAR reference should stay near the frame center.");
    expectNear(result.mid_reference.lateral_offset_px, 0.0, 6.0,
               "Centered MID reference should stay near the frame center.");
    expectNear(result.near_reference.lateral_offset_px, 0.0, 6.0,
               "Centered NEAR reference should stay near the frame center.");
    expect(result.heading_valid, "Centered lane should expose a valid heading.");
    expect(result.curvature_valid, "Centered lane should expose a valid curvature estimate.");
    expectNear(result.heading_error_rad, 0.0, 0.08,
               "Centered lane should have near-zero heading error.");
    expectNear(result.curvature_indicator_rad, 0.0, 0.08,
               "Centered lane should have near-zero curvature indicator.");
}

void testShiftedLeftLane() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 120, 135, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Left-shifted lane should be detected.");
    expect(result.lane_center_ratio < 0.5, "Left-shifted lane ratio should be below 0.5.");
    expect(result.steering_error_normalized < 0.0,
           "Left-shifted lane should produce a negative normalized error.");
    expect(result.far_reference.steering_error_normalized < 0.0,
           "Left-shifted FAR reference should stay on the left.");
    expect(result.mid_reference.steering_error_normalized < 0.0,
           "Left-shifted MID reference should stay on the left.");
    expect(result.near_reference.steering_error_normalized < 0.0,
           "Left-shifted NEAR reference should stay on the left.");
}

void testShiftedRightLane() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makeLaneFrame(320, 240, 205, 190, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Right-shifted lane should be detected.");
    expect(result.lane_center_ratio > 0.5, "Right-shifted lane ratio should be above 0.5.");
    expect(result.steering_error_normalized > 0.0,
           "Right-shifted lane should produce a positive normalized error.");
    expect(result.far_reference.steering_error_normalized > 0.0,
           "Right-shifted FAR reference should stay on the right.");
    expect(result.mid_reference.steering_error_normalized > 0.0,
           "Right-shifted MID reference should stay on the right.");
    expect(result.near_reference.steering_error_normalized > 0.0,
           "Right-shifted NEAR reference should stay on the right.");
}

void testCurvedLaneLikeShape() {
    LabConfig config = makeBaseConfig();
    const cv::Mat frame = makePolylineLaneFrame(320, 240, {260, 240, 145, 135, 135}, 110);
    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.lane_found, "Curved/slanted lane should still be detected.");
    expect(result.heading_valid, "Curved lane should expose a valid heading.");
    expect(result.curvature_valid, "Curved lane should expose a valid curvature estimate.");
    expect(result.heading_error_rad > 0.0,
           "Curved lane heading should be positive when the road trends to the right.");
    expect(result.curvature_indicator_rad > 0.0,
           "Curved lane curvature should be positive when the turn intensifies to the right.");
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
    expect(result.far_reference.top_y == 120 && result.near_reference.bottom_y == 239,
           "Lookahead ranges should be expressed in absolute frame coordinates.");
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

void testReferenceConfigParsing() {
    LabConfig config = makeBaseConfig();
    std::vector<std::string> warnings;
    const std::filesystem::path path = writeTempConfigFile(
        "road_segmentation_lab_reference_valid.env",
        "LANE_REFERENCE_FAR_TOP_RATIO=0.10\n"
        "LANE_REFERENCE_FAR_BOTTOM_RATIO=0.25\n"
        "LANE_REFERENCE_MID_TOP_RATIO=0.25\n"
        "LANE_REFERENCE_MID_BOTTOM_RATIO=0.60\n"
        "LANE_REFERENCE_NEAR_TOP_RATIO=0.60\n"
        "LANE_REFERENCE_NEAR_BOTTOM_RATIO=0.95\n");

    const bool loaded = loadConfigFromFile(path.string(), config, &warnings);
    std::filesystem::remove(path);

    expect(loaded, "Valid reference-zone config should load.");
    expect(warnings.empty(), "Valid reference-zone config should not emit warnings.");
    expectNear(config.reference_far_top_ratio, 0.10, 1e-6, "FAR top ratio should parse correctly.");
    expectNear(config.reference_mid_bottom_ratio, 0.60, 1e-6,
               "MID bottom ratio should parse correctly.");
    expectNear(config.reference_near_bottom_ratio, 0.95, 1e-6,
               "NEAR bottom ratio should parse correctly.");
}

void testReferenceConfigFallback() {
    LabConfig config = makeBaseConfig();
    std::vector<std::string> warnings;
    const std::filesystem::path path = writeTempConfigFile(
        "road_segmentation_lab_reference_invalid.env",
        "LANE_REFERENCE_FAR_TOP_RATIO=0.40\n"
        "LANE_REFERENCE_FAR_BOTTOM_RATIO=0.20\n"
        "LANE_REFERENCE_MID_TOP_RATIO=0.20\n"
        "LANE_REFERENCE_MID_BOTTOM_RATIO=0.50\n"
        "LANE_REFERENCE_NEAR_TOP_RATIO=0.50\n"
        "LANE_REFERENCE_NEAR_BOTTOM_RATIO=0.80\n");

    const bool loaded = loadConfigFromFile(path.string(), config, &warnings);
    std::filesystem::remove(path);

    expect(loaded, "Invalid reference-zone config should still load with fallback.");
    expect(!warnings.empty(), "Invalid reference-zone config should emit a warning.");
    expectNear(config.reference_far_top_ratio, 0.0, 1e-6,
               "Invalid reference ranges should restore FAR top default.");
    expectNear(config.reference_far_bottom_ratio, 0.33, 1e-6,
               "Invalid reference ranges should restore FAR bottom default.");
    expectNear(config.reference_mid_top_ratio, 0.33, 1e-6,
               "Invalid reference ranges should restore MID top default.");
    expectNear(config.reference_mid_bottom_ratio, 0.66, 1e-6,
               "Invalid reference ranges should restore MID bottom default.");
    expectNear(config.reference_near_top_ratio, 0.66, 1e-6,
               "Invalid reference ranges should restore NEAR top default.");
    expectNear(config.reference_near_bottom_ratio, 1.0, 1e-6,
               "Invalid reference ranges should restore NEAR bottom default.");
}

void testMissingFarReferenceDisablesCurvature() {
    LabConfig config = makeBaseConfig();
    cv::Mat frame = makeLaneFrame(320, 240, 160, 160, 120);
    cv::rectangle(frame, {0, 0}, {frame.cols - 1, 160}, cv::Scalar(255, 255, 255), cv::FILLED);

    const RoadSegmentationResult result = runPipeline(config, frame);

    expect(result.mid_reference.valid, "MID reference should remain valid when only FAR is missing.");
    expect(result.near_reference.valid, "NEAR reference should remain valid when only FAR is missing.");
    expect(!result.far_reference.valid, "FAR reference should become invalid when its band is empty.");
    expect(result.heading_valid, "Heading should still be valid with NEAR and MID references.");
    expect(!result.curvature_valid, "Curvature should be disabled when FAR reference is missing.");
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
        {"reference_config_parsing", testReferenceConfigParsing},
        {"reference_config_fallback", testReferenceConfigFallback},
        {"missing_far_reference_disables_curvature", testMissingFarReferenceDisablesCurvature},
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
