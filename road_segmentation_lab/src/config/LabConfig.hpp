#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace road_segmentation_lab::config {

enum class SegmentationMode {
    HsvDark,
    GrayThreshold,
    LabDark,
    AdaptiveGray,
};

struct LabConfig {
    bool resize_enabled{true};
    int target_width{320};
    int target_height{240};
    double roi_top_ratio{0.5};
    double roi_bottom_ratio{1.0};
    double roi_polygon_top_width_ratio{0.58};
    double roi_polygon_bottom_width_ratio{1.0};
    double roi_polygon_center_x_ratio{0.5};
    bool hood_mask_enabled{true};
    double hood_mask_width_ratio{0.34};
    double hood_mask_height_ratio{0.22};
    double hood_mask_center_x_ratio{0.5};
    double hood_mask_bottom_offset_ratio{0.02};
    bool gaussian_enabled{true};
    cv::Size gaussian_kernel{5, 5};
    double gaussian_sigma{1.2};
    bool clahe_enabled{false};
    SegmentationMode segmentation_mode{SegmentationMode::GrayThreshold};
    cv::Scalar hsv_low{0, 0, 0};
    cv::Scalar hsv_high{180, 110, 200};
    int gray_threshold{90};
    int adaptive_block_size{31};
    double adaptive_c{9.0};
    bool morph_enabled{true};
    cv::Size morph_kernel{5, 5};
    int morph_iterations{1};
    double min_contour_area{400.0};
    std::string calibration_file;
};

std::string segmentationModeToString(SegmentationMode mode);
bool loadConfigFromFile(const std::string &path, LabConfig &config,
                        std::vector<std::string> *warnings = nullptr);

} // namespace road_segmentation_lab::config
