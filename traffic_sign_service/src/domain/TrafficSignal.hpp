#pragma once

#include <optional>
#include <string_view>

namespace traffic_sign_service {

enum class TrafficSignalId { Stop, TurnLeft, TurnRight, Unknown };

std::string_view toString(TrafficSignalId signal_id);
std::optional<TrafficSignalId> trafficSignalIdFromCanonicalString(std::string_view value);
std::string displayLabel(TrafficSignalId signal_id);

} // namespace traffic_sign_service
