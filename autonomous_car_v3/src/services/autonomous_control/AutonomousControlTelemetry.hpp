#pragma once

#include <string>

#include "services/autonomous_control/AutonomousControlTypes.hpp"

namespace autonomous_car::services::autonomous_control {

std::string buildAutonomousControlTelemetryJson(const AutonomousControlSnapshot &snapshot);

} // namespace autonomous_car::services::autonomous_control
