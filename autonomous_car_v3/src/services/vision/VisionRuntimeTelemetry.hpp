#pragma once

#include <cstdint>
#include <string>

namespace autonomous_car::services::vision {

struct VisionRuntimeTelemetry {
    std::int64_t timestamp_ms{0};
    std::string source;
    double core_fps{0.0};
    double stream_fps{0.0};
    double traffic_sign_fps{0.0};
    double traffic_sign_inference_ms{0.0};
    double stream_encode_ms{0.0};
    std::uint64_t traffic_sign_dropped_frames{0};
    std::uint64_t stream_dropped_frames{0};
    std::int64_t sign_result_age_ms{-1};
};

std::string buildVisionRuntimeTelemetryJson(const VisionRuntimeTelemetry &telemetry);

} // namespace autonomous_car::services::vision
