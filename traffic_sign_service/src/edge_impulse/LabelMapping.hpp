#pragma once

#include <string_view>

#include "domain/TrafficSignal.hpp"

namespace traffic_sign_service::edge_impulse {

TrafficSignalId trafficSignalIdFromModelLabel(std::string_view label);

} // namespace traffic_sign_service::edge_impulse
