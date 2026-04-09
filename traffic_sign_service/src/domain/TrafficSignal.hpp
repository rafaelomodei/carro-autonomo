#pragma once

#include <optional>
#include <string_view>

namespace traffic_sign_service {

enum class TrafficSignalId { Stop, TurnLeft, TurnRight };

std::string_view toString(TrafficSignalId signal_id);
std::optional<TrafficSignalId> trafficSignalIdFromCanonicalString(std::string_view value);

} // namespace traffic_sign_service
