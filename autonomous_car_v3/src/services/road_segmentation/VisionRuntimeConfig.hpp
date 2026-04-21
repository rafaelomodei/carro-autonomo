#pragma once

#include <string>
#include <vector>

namespace autonomous_car::services::road_segmentation {

enum class VisionSourceMode {
    Camera,
    Video,
    Image,
};

struct VisionRuntimeConfig {
    VisionSourceMode source_mode{VisionSourceMode::Camera};
    std::string source_path;
    int camera_index{0};
    bool debug_window_enabled{true};
    bool traffic_sign_debug_window_enabled{false};
    double telemetry_max_fps{10.0};
    double stream_max_fps{5.0};
    double traffic_sign_target_fps{4.0};
    int stream_jpeg_quality{70};
    std::string segmentation_config_path;
    std::string traffic_sign_config_path;
};

std::string toString(VisionSourceMode mode);
bool loadVisionRuntimeConfigFromFile(const std::string &path, VisionRuntimeConfig &config,
                                     std::vector<std::string> *warnings = nullptr);

} // namespace autonomous_car::services::road_segmentation
