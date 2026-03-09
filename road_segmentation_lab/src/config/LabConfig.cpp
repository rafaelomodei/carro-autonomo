#include "config/LabConfig.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

namespace road_segmentation_lab::config {
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

int ensureOddPositive(int value, int fallback) {
    if (value <= 0) {
        value = fallback;
    }
    if (value % 2 == 0) {
        value += 1;
    }
    return std::max(1, value);
}

std::optional<cv::Size> parseKernel(const std::string &value) {
    std::string normalized = value;
    for (char &ch : normalized) {
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '-')) {
            ch = ' ';
        }
    }

    std::istringstream stream(normalized);
    int first = 0;
    int second = 0;
    if (!(stream >> first)) {
        return std::nullopt;
    }
    if (!(stream >> second)) {
        second = first;
    }

    return cv::Size(ensureOddPositive(first, 5), ensureOddPositive(second, 5));
}

std::optional<cv::Scalar> parseScalar(const std::string &value) {
    std::string normalized = value;
    for (char &ch : normalized) {
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-')) {
            ch = ' ';
        }
    }

    std::istringstream stream(normalized);
    double components[3] = {0.0, 0.0, 0.0};
    for (double &component : components) {
        if (!(stream >> component)) {
            return std::nullopt;
        }
        component = std::clamp(component, 0.0, 255.0);
    }

    return cv::Scalar(components[0], components[1], components[2]);
}

std::optional<SegmentationMode> parseSegmentationMode(const std::string &value) {
    if (iequals(value, "HSV_DARK")) {
        return SegmentationMode::HsvDark;
    }
    if (iequals(value, "GRAY_THRESHOLD")) {
        return SegmentationMode::GrayThreshold;
    }
    if (iequals(value, "LAB_DARK")) {
        return SegmentationMode::LabDark;
    }
    if (iequals(value, "ADAPTIVE_GRAY")) {
        return SegmentationMode::AdaptiveGray;
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

void pushWarning(std::vector<std::string> *warnings, const std::string &message) {
    if (warnings) {
        warnings->push_back(message);
    }
}

} // namespace

std::string segmentationModeToString(SegmentationMode mode) {
    switch (mode) {
    case SegmentationMode::HsvDark:
        return "HSV_DARK";
    case SegmentationMode::GrayThreshold:
        return "GRAY_THRESHOLD";
    case SegmentationMode::LabDark:
        return "LAB_DARK";
    case SegmentationMode::AdaptiveGray:
        return "ADAPTIVE_GRAY";
    }
    return "UNKNOWN";
}

bool loadConfigFromFile(const std::string &path, LabConfig &config,
                        std::vector<std::string> *warnings) {
    std::ifstream file(path);
    if (!file.is_open()) {
        pushWarning(warnings, "Nao foi possivel abrir o arquivo de configuracao: " + path);
        return false;
    }

    const std::filesystem::path config_path(path);
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

        if (key == "LANE_RESIZE_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.resize_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_TARGET_WIDTH") {
            if (const auto parsed = parseInt(value)) {
                config.target_width = std::max(1, *parsed);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_TARGET_HEIGHT") {
            if (const auto parsed = parseInt(value)) {
                config.target_height = std::max(1, *parsed);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ROI_TOP_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_top_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ROI_BOTTOM_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_bottom_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ROI_POLYGON_TOP_WIDTH_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_polygon_top_width_ratio = std::clamp(*parsed, 0.05, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ROI_POLYGON_BOTTOM_WIDTH_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_polygon_bottom_width_ratio = std::clamp(*parsed, 0.05, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ROI_POLYGON_CENTER_X_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.roi_polygon_center_x_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HOOD_MASK_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.hood_mask_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HOOD_MASK_WIDTH_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.hood_mask_width_ratio = std::clamp(*parsed, 0.05, 0.95);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HOOD_MASK_HEIGHT_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.hood_mask_height_ratio = std::clamp(*parsed, 0.05, 0.95);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HOOD_MASK_CENTER_X_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.hood_mask_center_x_ratio = std::clamp(*parsed, 0.0, 1.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HOOD_MASK_BOTTOM_OFFSET_RATIO") {
            if (const auto parsed = parseDouble(value)) {
                config.hood_mask_bottom_offset_ratio = std::clamp(*parsed, 0.0, 0.4);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_GAUSSIAN_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.gaussian_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_GAUSSIAN_KERNEL") {
            if (const auto parsed = parseKernel(value)) {
                config.gaussian_kernel = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_GAUSSIAN_SIGMA") {
            if (const auto parsed = parseDouble(value)) {
                config.gaussian_sigma = std::clamp(*parsed, 0.0, 50.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_CLAHE_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.clahe_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_SEGMENTATION_MODE") {
            if (const auto parsed = parseSegmentationMode(value)) {
                config.segmentation_mode = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HSV_LOW") {
            if (const auto parsed = parseScalar(value)) {
                config.hsv_low = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_HSV_HIGH") {
            if (const auto parsed = parseScalar(value)) {
                config.hsv_high = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_GRAY_THRESHOLD") {
            if (const auto parsed = parseInt(value)) {
                config.gray_threshold = std::clamp(*parsed, 0, 255);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ADAPTIVE_BLOCK_SIZE") {
            if (const auto parsed = parseInt(value)) {
                config.adaptive_block_size = ensureOddPositive(*parsed, config.adaptive_block_size);
                if (config.adaptive_block_size < 3) {
                    config.adaptive_block_size = 3;
                }
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_ADAPTIVE_C") {
            if (const auto parsed = parseDouble(value)) {
                config.adaptive_c = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_MORPH_ENABLED") {
            if (const auto parsed = parseBool(value)) {
                config.morph_enabled = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_MORPH_KERNEL") {
            if (const auto parsed = parseKernel(value)) {
                config.morph_kernel = *parsed;
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_MORPH_ITERATIONS") {
            if (const auto parsed = parseInt(value)) {
                config.morph_iterations = std::clamp(*parsed, 1, 10);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_MIN_CONTOUR_AREA") {
            if (const auto parsed = parseDouble(value)) {
                config.min_contour_area = std::clamp(*parsed, 10.0, 500000.0);
            } else {
                pushWarning(warnings, "Valor invalido para " + key);
            }
            continue;
        }

        if (key == "LANE_CALIBRATION_FILE") {
            config.calibration_file = resolveMaybeRelativePath(config_path, value);
            continue;
        }

        pushWarning(warnings, "Chave desconhecida ignorada: " + key);
    }

    if (config.roi_bottom_ratio <= config.roi_top_ratio) {
        config.roi_bottom_ratio = std::min(1.0, config.roi_top_ratio + 0.05);
        if (config.roi_bottom_ratio <= config.roi_top_ratio) {
            config.roi_top_ratio = std::max(0.0, config.roi_bottom_ratio - 0.05);
        }
    }

    if (config.roi_polygon_top_width_ratio > config.roi_polygon_bottom_width_ratio) {
        config.roi_polygon_top_width_ratio = config.roi_polygon_bottom_width_ratio;
    }

    return true;
}

} // namespace road_segmentation_lab::config
