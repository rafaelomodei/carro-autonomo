#pragma once

#include <string>
#include <string_view>

#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

std::string buildTrafficSignTelemetryJson(const TrafficSignFrameResult &result,
                                          std::string_view source);

} // namespace autonomous_car::services::traffic_sign_detection
