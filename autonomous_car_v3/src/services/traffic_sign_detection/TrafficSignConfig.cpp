#include "services/traffic_sign_detection/TrafficSignConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <optional>

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
    for (const char ch : value) {
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

        if (key == "TRAFFIC_SIGN_ROI_RIGHT_WIDTH_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_right_width_ratio = std::clamp(*parsed, 0.05, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_TOP_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_top_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_ROI_BOTTOM_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_bottom_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONFIDENCE") {
            if (const auto parsed = parseDouble(value)) {
                config.min_confidence = std::clamp(*parsed, 0.01, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES") {
            if (const auto parsed = parseInt(value)) {
                config.min_consecutive_frames = std::clamp(*parsed, 1, 120);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "TRAFFIC_SIGN_MAX_MISSED_FRAMES") {
            if (const auto parsed = parseInt(value)) {
                config.max_missed_frames = std::clamp(*parsed, 0, 120);
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

        pushWarning(warnings, "Chave desconhecida em traffic_sign_detection.env: " + key);
    }

    if (config.roi_bottom_ratio <= config.roi_top_ratio) {
        pushWarning(warnings,
                    "TRAFFIC_SIGN_ROI_BOTTOM_RATIO deve ser maior que TRAFFIC_SIGN_ROI_TOP_RATIO."
                    " Ajustando para um minimo valido.");
        config.roi_bottom_ratio = std::min(1.0, config.roi_top_ratio + 0.1);
    }

    return true;
}

cv::Rect computeTrafficSignRoi(const cv::Size &frame_size, const TrafficSignConfig &config) {
    if (frame_size.width <= 0 || frame_size.height <= 0) {
        return {};
    }

    const int width = std::clamp(
        static_cast<int>(std::lround(frame_size.width * config.roi_right_width_ratio)), 1,
        frame_size.width);
    const int x = std::max(0, frame_size.width - width);
    const int top =
        std::clamp(static_cast<int>(std::lround(frame_size.height * config.roi_top_ratio)), 0,
                   frame_size.height - 1);
    const int bottom = std::clamp(
        static_cast<int>(std::lround(frame_size.height * config.roi_bottom_ratio)), top + 1,
        frame_size.height);

    return {x, top, width, std::max(1, bottom - top)};
}

} // namespace autonomous_car::services::traffic_sign_detection
