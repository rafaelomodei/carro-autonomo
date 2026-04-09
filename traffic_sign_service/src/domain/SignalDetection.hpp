#pragma once

#include "domain/TrafficSignal.hpp"

namespace traffic_sign_service {

struct SignalDetection {
    TrafficSignalId signal_id{TrafficSignalId::Stop};
    double confidence{0.0};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

} // namespace traffic_sign_service
