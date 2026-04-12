#include "services/traffic_sign_detection/TrafficSignConfig.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <string>

namespace autonomous_car::services::traffic_sign_detection {
namespace {

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

void pushWarning(std::vector<std::string> *warnings, const std::string &message) {
    if (warnings) {
        warnings->push_back(message);
    }
}

std::optional<bool> parseBool(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        normalized.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
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

} // namespace

bool loadTrafficSignConfigFromFile(const std::string &path, TrafficSignConfig &config,
                                   std::vector<std::string> *warnings) {
    std::ifstream file(path);
    if (!file.is_open()) {
        pushWarning(warnings,
                    "Nao foi possivel abrir o arquivo de configuracao de sinalizacao: " + path);
        return false;
    }

    bool roi_left_configured = false;
    bool roi_right_configured = false;
    std::optional<double> legacy_right_width_ratio;

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

        if (key == "TRAFFIC_SIGN_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_LEFT_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_left_ratio = std::clamp(*parsed, 0.0, 1.0);
                roi_left_configured = true;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_RIGHT_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_right_ratio = std::clamp(*parsed, 0.0, 1.0);
                roi_right_configured = true;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_RIGHT_WIDTH_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                legacy_right_width_ratio = std::clamp(*parsed, 0.05, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_TOP_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_top_ratio = std::clamp(*parsed, 0.0, 0.95);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_BOTTOM_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_bottom_ratio = std::clamp(*parsed, 0.01, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_DEBUG_ROI_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.debug_roi_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONFIDENCE") {
            if (const auto parsed = parseDouble(value)) {
                config.min_confidence = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES") {
            if (const auto parsed = parseInt(value)) {
                config.min_consecutive_frames = std::clamp(*parsed, 1, 30);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MAX_MISSED_FRAMES") {
            if (const auto parsed = parseInt(value)) {
                config.max_missed_frames = std::clamp(*parsed, 0, 30);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MAX_RAW_DETECTIONS") {
            if (const auto parsed = parseInt(value)) {
                config.max_raw_detections = std::clamp(*parsed, 1, 20);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        pushWarning(warnings, "Chave desconhecida em traffic_sign.env: " + key);
    }

    if (!roi_left_configured && !roi_right_configured && legacy_right_width_ratio.has_value()) {
        config.roi_left_ratio = std::clamp(1.0 - *legacy_right_width_ratio, 0.0, 0.95);
        config.roi_right_ratio = 1.0;
    }

    if (config.roi_right_ratio <= config.roi_left_ratio) {
        pushWarning(warnings,
                    "TRAFFIC_SIGN_ROI_RIGHT_RATIO deve ser maior que "
                    "TRAFFIC_SIGN_ROI_LEFT_RATIO. Ajustando para um minimo valido.");
        config.roi_right_ratio = std::clamp(config.roi_left_ratio + 0.05, 0.05, 1.0);
        if (config.roi_right_ratio <= config.roi_left_ratio) {
            config.roi_left_ratio = std::max(0.0, config.roi_right_ratio - 0.05);
        }
    }

    if (config.roi_bottom_ratio <= config.roi_top_ratio) {
        pushWarning(warnings,
                    "TRAFFIC_SIGN_ROI_BOTTOM_RATIO deve ser maior que "
                    "TRAFFIC_SIGN_ROI_TOP_RATIO. Ajustando para um minimo valido.");
        config.roi_bottom_ratio = std::clamp(config.roi_top_ratio + 0.05, 0.06, 1.0);
    }

    return true;
}

} // namespace autonomous_car::services::traffic_sign_detection
