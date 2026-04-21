#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service::config {

struct ServiceConfig {
    FrameSourceMode frame_source_mode{FrameSourceMode::WebSocket};
    std::string frame_source_path;
    int camera_index{0};
    std::string vehicle_ws_url{"ws://192.168.15.163:8080"};
    std::string edge_impulse_model_zip_path{
        "../edgeImpulse/tcc-pare-direita-esquerda-cam-raspberry-cpp-linux-v20-impulse.zip"};
    std::string edge_impulse_eim_path;
    EdgeImpulseBackend edge_impulse_backend{EdgeImpulseBackend::Eim};
    bool eim_print_info_on_start{true};
    bool traffic_sign_preprocess_grayscale{false};
    double traffic_sign_roi_left_ratio{0.55};
    double traffic_sign_roi_right_ratio{1.0};
    double traffic_sign_roi_top_ratio{0.08};
    double traffic_sign_roi_bottom_ratio{0.72};
    bool traffic_sign_debug_roi_enabled{true};
    double traffic_sign_min_confidence{0.60};
    int traffic_sign_min_consecutive_frames{2};
    int traffic_sign_max_missed_frames{3};
    int traffic_sign_max_raw_detections{3};
    std::uint64_t detection_cooldown_ms{2000};
    double inference_max_fps{5.0};
    bool frame_preview_enabled{false};
};

bool loadServiceConfigFromFile(const std::string &path, ServiceConfig &config,
                               std::vector<std::string> *warnings = nullptr);

} // namespace traffic_sign_service::config
