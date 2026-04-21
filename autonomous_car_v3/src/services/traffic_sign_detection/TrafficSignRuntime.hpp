#pragma once

#include <cstdint>

#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

std::int64_t trafficSignResultAgeMs(const TrafficSignFrameResult &result,
                                    std::int64_t reference_timestamp_ms);

TrafficSignFrameResult buildRenderableTrafficSignResult(const TrafficSignFrameResult &result,
                                                        std::int64_t reference_timestamp_ms,
                                                        std::int64_t max_age_ms);

} // namespace autonomous_car::services::traffic_sign_detection
