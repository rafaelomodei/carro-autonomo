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

        if (key == "DETECTION_MIN_CONFIDENCE") {
            const auto parsed = parseDouble(value);
            if (!parsed || *parsed <= 0.0 || *parsed > 1.0) {
                pushWarning(warnings, "DETECTION_MIN_CONFIDENCE invalido: " + value);
                success = false;
                continue;
            }
            loaded.detection_min_confidence = *parsed;
            continue;
        }

        if (key == "DETECTION_CONFIRMATION_FRAMES") {
            const auto parsed = parseSize(value);
            if (!parsed || *parsed == 0) {
                pushWarning(warnings, "DETECTION_CONFIRMATION_FRAMES invalido: " + value);
                success = false;
                continue;
            }
            loaded.detection_confirmation_frames = *parsed;
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

    config = loaded;
    return success;
}

} // namespace traffic_sign_service::config
