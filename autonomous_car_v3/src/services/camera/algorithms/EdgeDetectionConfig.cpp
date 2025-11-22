#include "services/camera/algorithms/EdgeDetectionConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace autonomous_car::services::camera::algorithms {
namespace {
bool TryGetEnv(const char *name, std::string &value) {
    const char *raw = std::getenv(name);
    if (!raw) {
        return false;
    }
    value = raw;
    return true;
}

double ReadDouble(const char *name, double fallback, double min_value, double max_value) {
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

int ReadAperture(const char *name, int fallback) {
    const int parsed = static_cast<int>(ReadDouble(name, fallback, 3.0, 7.0));
    if (parsed % 2 == 0) {
        return fallback;
    }
    return parsed;
}

bool ReadBool(const char *name, bool fallback) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    for (auto &ch : raw) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    if (raw == "true" || raw == "1" || raw == "yes" || raw == "on") {
        return true;
    }
    if (raw == "false" || raw == "0" || raw == "no" || raw == "off") {
        return false;
    }
    return fallback;
}

cv::Size ReadKernel(const char *name, cv::Size fallback) {
    std::string raw;
    if (!TryGetEnv(name, raw)) {
        return fallback;
    }

    int width = fallback.width;
    int height = fallback.height;
    int parsed = 0;
    if (std::sscanf(raw.c_str(), "%d%*[^0-9]%d", &width, &height) >= 1) {
        parsed = width;
    }

    if (parsed > 0) {
        if (height <= 0) {
            height = width;
        }
        if (width % 2 == 0) {
            width += 1;
        }
        if (height % 2 == 0) {
            height += 1;
        }
        return {std::max(1, width), std::max(1, height)};
    }

    return fallback;
}
} // namespace

EdgeDetectionConfig EdgeDetectionConfig::LoadFromEnv() {
    EdgeDetectionConfig config;
    config.canny_threshold1 =
        ReadDouble("EDGE_CANNY_THRESHOLD1", config.canny_threshold1, 0.0, 500.0);
    config.canny_threshold2 =
        ReadDouble("EDGE_CANNY_THRESHOLD2", config.canny_threshold2, 0.0, 500.0);
    config.canny_aperture = ReadAperture("EDGE_CANNY_APERTURE", config.canny_aperture);
    config.canny_l2gradient = ReadBool("EDGE_CANNY_L2GRADIENT", config.canny_l2gradient);
    config.blur_kernel = ReadKernel("EDGE_BLUR_KERNEL", config.blur_kernel);
    return config;
}

} // namespace autonomous_car::services::camera::algorithms
