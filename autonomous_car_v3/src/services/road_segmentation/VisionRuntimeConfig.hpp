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
    double telemetry_max_fps{10.0};
    std::string segmentation_config_path;
};

std::string toString(VisionSourceMode mode);
bool loadVisionRuntimeConfigFromFile(const std::string &path, VisionRuntimeConfig &config,
                                     std::vector<std::string> *warnings = nullptr);

} // namespace autonomous_car::services::road_segmentation
