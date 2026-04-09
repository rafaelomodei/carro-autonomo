#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace traffic_sign_service::config {

struct ServiceConfig {
    std::string vehicle_ws_url{"ws://192.168.15.163:8080"};
    std::string edge_impulse_model_zip_path{
        "../edgeImpulse/tcc-pare-direita-esquerda-cam-raspberry-v15.zip"};
    double detection_min_confidence{0.60};
    std::size_t detection_confirmation_frames{3};
    std::uint64_t detection_cooldown_ms{2000};
    double inference_max_fps{5.0};
    bool frame_preview_enabled{false};
};

bool loadServiceConfigFromFile(const std::string &path, ServiceConfig &config,
                               std::vector<std::string> *warnings = nullptr);

} // namespace traffic_sign_service::config
