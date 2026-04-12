#pragma once

#include <string>
#include <vector>

namespace autonomous_car::services::traffic_sign_detection {

struct TrafficSignConfig {
    bool enabled{true};
    double roi_left_ratio{0.55};
    double roi_right_ratio{1.0};
    double roi_top_ratio{0.08};
    double roi_bottom_ratio{0.72};
    bool debug_roi_enabled{true};
    double min_confidence{0.60};
    int min_consecutive_frames{2};
    int max_missed_frames{3};
    int max_raw_detections{3};
};

bool loadTrafficSignConfigFromFile(const std::string &path, TrafficSignConfig &config,
                                   std::vector<std::string> *warnings = nullptr);

} // namespace autonomous_car::services::traffic_sign_detection
