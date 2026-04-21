#include "config/ServiceConfig.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

namespace {

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

void pushWarning(std::vector<std::string> *warnings, const std::string &warning) {
    if (warnings) {
        warnings->push_back(warning);
    }
}

std::optional<double> parseDouble(const std::string &value) {
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(value, &consumed);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return parsed;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<int> parseInt(const std::string &value) {
    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed, 10);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return parsed;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<std::uint64_t> parseUnsignedLong(const std::string &value) {
    try {
        std::size_t consumed = 0;
        const unsigned long long parsed = std::stoull(value, &consumed, 10);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(parsed);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<std::size_t> parseSize(const std::string &value) {
    const auto parsed = parseUnsignedLong(value);
    if (!parsed) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*parsed);
}

std::optional<bool> parseBool(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "1" || normalized == "true" || normalized == "yes" ||
        normalized == "on") {
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "no" ||
        normalized == "off") {
        return false;
    }

    return std::nullopt;
}

bool isValidWebSocketUrl(const std::string &value) {
    return value.rfind("ws://", 0) == 0 || value.rfind("wss://", 0) == 0;
}

std::string resolvePath(const std::filesystem::path &config_path, const std::string &raw_path) {
    const std::filesystem::path candidate = raw_path;
    if (candidate.is_absolute()) {
        return candidate.string();
    }
    return std::filesystem::absolute(config_path.parent_path() / candidate).string();
}

} // namespace

namespace traffic_sign_service::config {

bool loadServiceConfigFromFile(const std::string &path, ServiceConfig &config,
                               std::vector<std::string> *warnings) {
    const std::filesystem::path config_path = std::filesystem::absolute(path);
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    ServiceConfig loaded = config;
    std::string line;
    bool success = true;

    while (std::getline(file, line)) {
        const auto trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const auto delimiter = trimmed.find('=');
        if (delimiter == std::string::npos) {
            pushWarning(warnings, "Linha ignorada sem '=': " + trimmed);
            success = false;
            continue;
        }

        const auto key = trim(trimmed.substr(0, delimiter));
        const auto value = trim(trimmed.substr(delimiter + 1));

        if (key == "FRAME_SOURCE_MODE") {
            const auto parsed = frameSourceModeFromString(value);
            if (!parsed) {
                pushWarning(warnings, "FRAME_SOURCE_MODE invalido: " + value);
                success = false;
                continue;
            }
            loaded.frame_source_mode = *parsed;
            continue;
        }

        if (key == "FRAME_SOURCE_PATH") {
            loaded.frame_source_path = resolvePath(config_path, value);
            continue;
        }

        if (key == "CAMERA_INDEX") {
            const auto parsed = parseInt(value);
            if (!parsed || *parsed < 0) {
                pushWarning(warnings, "CAMERA_INDEX invalido: " + value);
                success = false;
                continue;
            }
            loaded.camera_index = *parsed;
            continue;
        }

        if (key == "VEHICLE_WS_URL") {
            if (!isValidWebSocketUrl(value)) {
                pushWarning(warnings, "VEHICLE_WS_URL invalida: " + value);
                success = false;
                continue;
            }
            loaded.vehicle_ws_url = value;
            continue;
        }

        if (key == "EDGE_IMPULSE_MODEL_ZIP") {
            loaded.edge_impulse_model_zip_path = resolvePath(config_path, value);
            continue;
        }

        if (key == "EDGE_IMPULSE_EIM_PATH") {
            loaded.edge_impulse_eim_path = resolvePath(config_path, value);
            continue;
        }

        if (key == "EDGE_IMPULSE_BACKEND") {
            const auto parsed = edgeImpulseBackendFromString(value);
            if (!parsed) {
                pushWarning(warnings, "EDGE_IMPULSE_BACKEND invalido: " + value);
                success = false;
                continue;
            }
            loaded.edge_impulse_backend = *parsed;
            continue;
        }

        if (key == "EIM_PRINT_INFO_ON_START") {
            const auto parsed = parseBool(value);
            if (!parsed) {
                pushWarning(warnings, "EIM_PRINT_INFO_ON_START invalido: " + value);
                success = false;
                continue;
            }
            loaded.eim_print_info_on_start = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_PREPROCESS_GRAYSCALE" ||
            key == "TRAFFIC_SIGN_INPUT_GRAYSCALE") {
            const auto parsed = parseBool(value);
            if (!parsed) {
                pushWarning(warnings, key + " invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_preprocess_grayscale = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_LEFT_RATIO") {
            const auto parsed = parseDouble(value);
            if (!parsed) {
                pushWarning(warnings, "TRAFFIC_SIGN_ROI_LEFT_RATIO invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_roi_left_ratio = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_RIGHT_RATIO") {
            const auto parsed = parseDouble(value);
            if (!parsed) {
                pushWarning(warnings, "TRAFFIC_SIGN_ROI_RIGHT_RATIO invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_roi_right_ratio = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_TOP_RATIO") {
            const auto parsed = parseDouble(value);
            if (!parsed) {
                pushWarning(warnings, "TRAFFIC_SIGN_ROI_TOP_RATIO invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_roi_top_ratio = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_BOTTOM_RATIO") {
            const auto parsed = parseDouble(value);
            if (!parsed) {
                pushWarning(warnings, "TRAFFIC_SIGN_ROI_BOTTOM_RATIO invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_roi_bottom_ratio = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_DEBUG_ROI_ENABLED") {
            const auto parsed = parseBool(value);
            if (!parsed) {
                pushWarning(warnings, "TRAFFIC_SIGN_DEBUG_ROI_ENABLED invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_debug_roi_enabled = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONFIDENCE" || key == "DETECTION_MIN_CONFIDENCE") {
            const auto parsed = parseDouble(value);
            if (!parsed || *parsed < 0.0 || *parsed > 1.0) {
                pushWarning(warnings, key + " invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_min_confidence = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES" ||
            key == "DETECTION_CONFIRMATION_FRAMES") {
            const auto parsed = parseInt(value);
            if (!parsed || *parsed <= 0) {
                pushWarning(warnings, key + " invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_min_consecutive_frames = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_MAX_MISSED_FRAMES") {
            const auto parsed = parseInt(value);
            if (!parsed || *parsed < 0) {
                pushWarning(warnings, "TRAFFIC_SIGN_MAX_MISSED_FRAMES invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_max_missed_frames = *parsed;
            continue;
        }

        if (key == "TRAFFIC_SIGN_MAX_RAW_DETECTIONS") {
            const auto parsed = parseInt(value);
            if (!parsed || *parsed <= 0) {
                pushWarning(warnings, "TRAFFIC_SIGN_MAX_RAW_DETECTIONS invalido: " + value);
                success = false;
                continue;
            }
            loaded.traffic_sign_max_raw_detections = *parsed;
            continue;
        }

        if (key == "DETECTION_COOLDOWN_MS") {
            const auto parsed = parseUnsignedLong(value);
            if (!parsed) {
                pushWarning(warnings, "DETECTION_COOLDOWN_MS invalido: " + value);
                success = false;
                continue;
            }
            loaded.detection_cooldown_ms = *parsed;
            continue;
        }

        if (key == "INFERENCE_MAX_FPS") {
            const auto parsed = parseDouble(value);
            if (!parsed || *parsed <= 0.0) {
                pushWarning(warnings, "INFERENCE_MAX_FPS invalido: " + value);
                success = false;
                continue;
            }
            loaded.inference_max_fps = *parsed;
            continue;
        }

        if (key == "FRAME_PREVIEW_ENABLED") {
            const auto parsed = parseBool(value);
            if (!parsed) {
                pushWarning(warnings, "FRAME_PREVIEW_ENABLED invalido: " + value);
                success = false;
                continue;
            }
            loaded.frame_preview_enabled = *parsed;
            continue;
        }

        pushWarning(warnings, "Chave desconhecida em service.env: " + key);
    }

    if (loaded.edge_impulse_model_zip_path.empty()) {
        pushWarning(warnings, "EDGE_IMPULSE_MODEL_ZIP nao pode ficar vazio.");
        success = false;
    }

    loaded.traffic_sign_roi_left_ratio =
        std::clamp(loaded.traffic_sign_roi_left_ratio, 0.0, 0.95);
    loaded.traffic_sign_roi_right_ratio =
        std::clamp(loaded.traffic_sign_roi_right_ratio,
                   loaded.traffic_sign_roi_left_ratio + 0.01, 1.0);
    loaded.traffic_sign_roi_top_ratio =
        std::clamp(loaded.traffic_sign_roi_top_ratio, 0.0, 0.95);
    loaded.traffic_sign_roi_bottom_ratio =
        std::clamp(loaded.traffic_sign_roi_bottom_ratio,
                   loaded.traffic_sign_roi_top_ratio + 0.01, 1.0);
    loaded.traffic_sign_min_consecutive_frames =
        std::clamp(loaded.traffic_sign_min_consecutive_frames, 1, 30);
    loaded.traffic_sign_max_missed_frames =
        std::clamp(loaded.traffic_sign_max_missed_frames, 0, 30);
    loaded.traffic_sign_max_raw_detections =
        std::clamp(loaded.traffic_sign_max_raw_detections, 1, 20);

    if ((loaded.frame_source_mode == FrameSourceMode::Image ||
         loaded.frame_source_mode == FrameSourceMode::Video) &&
        loaded.frame_source_path.empty()) {
        pushWarning(warnings, "FRAME_SOURCE_PATH nao pode ficar vazio para image/video.");
        success = false;
    }

    if (loaded.frame_source_mode == FrameSourceMode::WebSocket &&
        !isValidWebSocketUrl(loaded.vehicle_ws_url)) {
        pushWarning(warnings, "VEHICLE_WS_URL invalida para modo websocket.");
        success = false;
    }

    if (loaded.edge_impulse_backend == EdgeImpulseBackend::Eim &&
        loaded.edge_impulse_eim_path.empty()) {
        pushWarning(warnings, "EDGE_IMPULSE_EIM_PATH nao pode ficar vazio com backend eim.");
        success = false;
    }

    config = loaded;
    return success;
}

} // namespace traffic_sign_service::config
