#pragma once

#include <string>
#include <string_view>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service::telemetry {

std::string buildTrafficSignTelemetryJson(const TrafficSignFrameResult &result,
                                          std::string_view source);

} // namespace traffic_sign_service::telemetry
