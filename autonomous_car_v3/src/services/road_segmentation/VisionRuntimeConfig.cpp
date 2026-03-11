#include "services/road_segmentation/VisionRuntimeConfig.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

namespace autonomous_car::services::road_segmentation {
namespace {

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool iequals(const std::string &lhs, const std::string &rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }

    return true;
}

void pushWarning(std::vector<std::string> *warnings, const std::string &message) {
    if (warnings) {
        warnings->push_back(message);
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

std::optional<bool> parseBool(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return std::nullopt;
}

std::optional<VisionSourceMode> parseSourceMode(const std::string &value) {
    if (iequals(value, "camera")) {
        return VisionSourceMode::Camera;
    }
    if (iequals(value, "video")) {
        return VisionSourceMode::Video;
    }
    if (iequals(value, "image")) {
        return VisionSourceMode::Image;
    }
    return std::nullopt;
}

std::string resolveMaybeRelativePath(const std::filesystem::path &config_path,
                                     const std::string &raw_value) {
    if (raw_value.empty()) {
        return {};
    }

    const std::filesystem::path raw_path(raw_value);
    if (raw_path.is_absolute()) {
        return raw_path.string();
    }

    return std::filesystem::absolute(config_path.parent_path() / raw_path).string();
}

} // namespace

std::string toString(VisionSourceMode mode) {
    switch (mode) {
    case VisionSourceMode::Camera:
        return "camera";
    case VisionSourceMode::Video:
        return "video";
    case VisionSourceMode::Image:
        return "image";
    }

    return "camera";
}

bool loadVisionRuntimeConfigFromFile(const std::string &path, VisionRuntimeConfig &config,
                                     std::vector<std::string> *warnings) {
    const std::filesystem::path config_path(path);
    if (config.segmentation_config_path.empty()) {
        config.segmentation_config_path =
            std::filesystem::absolute(config_path.parent_path() / "road_segmentation.env").string();
    }
    if (config.traffic_sign_config_path.empty()) {
        config.traffic_sign_config_path = std::filesystem::absolute(
                                              config_path.parent_path() /
                                              "traffic_sign_detection.env")
                                              .string();
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        pushWarning(warnings, "Nao foi possivel abrir o arquivo de configuracao de visao: " + path);
        return false;
    }

    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        const std::string trimmed_line = trim(line);
        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue;
        }

        const auto delimiter = trimmed_line.find('=');
        if (delimiter == std::string::npos) {
            pushWarning(warnings, "Linha invalida " + std::to_string(line_number) + ": " +
                                      trimmed_line);
            continue;
        }

        const std::string key = trim(trimmed_line.substr(0, delimiter));
        const std::string value = trim(trimmed_line.substr(delimiter + 1));

        if (key == "VISION_SOURCE_MODE") {
            if (const auto parsed = parseSourceMode(value)) {
                config.source_mode = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_SOURCE_PATH") {
            config.source_path = resolveMaybeRelativePath(config_path, value);
            continue;
        }

        if (key == "VISION_CAMERA_INDEX") {
            if (const auto parsed = parseInt(value)) {
                config.camera_index = std::max(0, *parsed);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_DEBUG_WINDOW_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.debug_window_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_TELEMETRY_MAX_FPS") {
            if (const auto parsed = parseDouble(value)) {
                config.telemetry_max_fps = std::clamp(*parsed, 0.1, 120.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_STREAM_MAX_FPS") {
            if (const auto parsed = parseDouble(value)) {
                config.stream_max_fps = std::clamp(*parsed, 0.1, 60.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_STREAM_JPEG_QUALITY") {
            if (const auto parsed = parseInt(value)) {
                config.stream_jpeg_quality = std::clamp(*parsed, 10, 100);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "VISION_SEGMENTATION_CONFIG_PATH") {
            config.segmentation_config_path = resolveMaybeRelativePath(config_path, value);
            continue;
        }

        if (key == "VISION_TRAFFIC_SIGN_CONFIG_PATH") {
            config.traffic_sign_config_path = resolveMaybeRelativePath(config_path, value);
            continue;
        }

        pushWarning(warnings, "Chave desconhecida em vision.env: " + key);
    }

    if (config.source_mode != VisionSourceMode::Camera && config.source_path.empty()) {
        pushWarning(warnings,
                    "VISION_SOURCE_PATH vazio para modo nao-camera; o servico tentara fallback para camera.");
    }

    return true;
}

} // namespace autonomous_car::services::road_segmentation
