#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace autonomous_car::services::traffic_sign_detection {

struct TrafficSignConfig {
    bool enabled{true};
    double roi_right_width_ratio{0.35};
    double roi_top_ratio{0.0};
    double roi_bottom_ratio{1.0};
    double min_confidence{0.55};
    int min_consecutive_frames{3};
    int max_missed_frames{2};
    int max_raw_detections{5};
};

bool loadTrafficSignConfigFromFile(const std::string &path, TrafficSignConfig &config,
                                   std::vector<std::string> *warnings = nullptr);
cv::Rect computeTrafficSignRoi(const cv::Size &frame_size, const TrafficSignConfig &config);

} // namespace autonomous_car::services::traffic_sign_detection
