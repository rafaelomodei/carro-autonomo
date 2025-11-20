#include "services/camera/LaneFilterConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace autonomous_car::services::camera {
namespace {

bool TryGetEnv(const char *name, std::string &value) {
    const char *raw = std::getenv(name);
    if (!raw) {
        return false;
    }
    value = raw;
    return true;
}

double ReadClampedDouble(const char *name, double fallback, double min_value, double max_value) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    char *end = nullptr;
    const double parsed = std::strtod(raw.c_str(), &end);
    if (end == raw.c_str()) {
        return fallback;
    }

    return std::clamp(parsed, min_value, max_value);
}

int EnsureOddPositive(int value, int fallback) {
    if (value <= 0) {
        return fallback;
    }
    if (value % 2 == 0) {
        value += 1;
    }
    return value;
}

std::vector<double> ExtractNumbers(const std::string &raw) {
    std::string normalized = raw;
    for (char &ch : normalized) {
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-')) {
            ch = ' ';
        }
    }

    std::istringstream stream(normalized);
    std::vector<double> numbers;
    double value = 0.0;
    while (stream >> value) {
        numbers.push_back(value);
    }
    return numbers;
}

cv::Size ReadKernelSize(const char *name, cv::Size fallback) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    const auto values = ExtractNumbers(raw);
    if (values.empty()) {
        return fallback;
    }

    int width = static_cast<int>(values[0]);
    int height = values.size() > 1 ? static_cast<int>(values[1]) : width;
    width = EnsureOddPositive(width, fallback.width);
    height = EnsureOddPositive(height, fallback.height);
    return {width, height};
}

cv::Scalar ReadScalar(const char *name, const cv::Scalar &fallback) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    const auto values = ExtractNumbers(raw);
    if (values.empty()) {
        return fallback;
    }

    cv::Scalar result = fallback;
    for (int i = 0; i < 3 && i < static_cast<int>(values.size()); ++i) {
        result[i] = std::clamp(values[i], 0.0, 255.0);
    }
    return result;
}

int ReadInt(const char *name, int fallback, int min_value, int max_value) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    char *end = nullptr;
    const long parsed = std::strtol(raw.c_str(), &end, 10);
    if (end == raw.c_str()) {
        return fallback;
    }

    return static_cast<int>(std::clamp(parsed, static_cast<long>(min_value), static_cast<long>(max_value)));
}

} // namespace

LaneFilterConfig LaneFilterConfig::LoadFromEnv() {
    LaneFilterConfig config;
    config.roi_band_start_ratio =
        ReadClampedDouble("LANE_ROI_BAND_START", config.roi_band_start_ratio, 0.0, 1.0);
    config.roi_band_end_ratio =
        ReadClampedDouble("LANE_ROI_BAND_END", config.roi_band_end_ratio, 0.0, 1.0);
    if (config.roi_band_end_ratio <= config.roi_band_start_ratio) {
        config.roi_band_end_ratio = std::min(1.0, config.roi_band_start_ratio + 0.05);
        if (config.roi_band_end_ratio <= config.roi_band_start_ratio) {
            config.roi_band_start_ratio = std::max(0.0, config.roi_band_end_ratio - 0.05);
        }
    }
    config.gaussian_kernel = ReadKernelSize("LANE_GAUSSIAN_KERNEL", config.gaussian_kernel);
    config.gaussian_sigma = ReadClampedDouble("LANE_GAUSSIAN_SIGMA", config.gaussian_sigma, 0.0, 50.0);
    config.hsv_low = ReadScalar("LANE_HSV_LOW", config.hsv_low);
    config.hsv_high = ReadScalar("LANE_HSV_HIGH", config.hsv_high);
    config.morph_kernel = ReadKernelSize("LANE_MORPH_KERNEL", config.morph_kernel);
    config.morph_iterations =
        ReadInt("LANE_MORPH_ITERATIONS", config.morph_iterations, 1, 10);
    config.min_contour_area =
        ReadClampedDouble("LANE_MIN_CONTOUR_AREA", config.min_contour_area, 10.0, 50000.0);

    return config;
}

} // namespace autonomous_car::services::camera

