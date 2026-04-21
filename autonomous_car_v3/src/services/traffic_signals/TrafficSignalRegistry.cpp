#include "services/traffic_signals/TrafficSignalRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

std::string normalize(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

} // namespace

namespace autonomous_car::services::traffic_signals {

std::optional<TrafficSignalId> trafficSignalIdFromString(std::string_view value) {
    const auto normalized = normalize(value);
    if (normalized == "stop") {
        return TrafficSignalId::Stop;
    }
    if (normalized == "turn_left") {
        return TrafficSignalId::TurnLeft;
    }
    if (normalized == "turn_right") {
        return TrafficSignalId::TurnRight;
    }
    return std::nullopt;
}

std::string_view toString(TrafficSignalId signal_id) {
    switch (signal_id) {
    case TrafficSignalId::Stop:
        return "stop";
    case TrafficSignalId::TurnLeft:
        return "turn_left";
    case TrafficSignalId::TurnRight:
        return "turn_right";
    }
    return "unknown";
}

bool TrafficSignalRegistry::recordDetectedSignal(TrafficSignalId signal_id,
                                                 std::uint64_t received_at_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_detected_signal_ = TrafficSignalEvent{signal_id, received_at_ms};
    return true;
}

bool TrafficSignalRegistry::recordDetectedSignal(std::string_view raw_signal_id,
                                                 std::uint64_t received_at_ms) {
    const auto parsed_signal = trafficSignalIdFromString(raw_signal_id);
    if (!parsed_signal) {
        return false;
    }
    return recordDetectedSignal(*parsed_signal, received_at_ms);
}

std::optional<TrafficSignalEvent> TrafficSignalRegistry::lastDetectedSignal() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_detected_signal_;
}

} // namespace autonomous_car::services::traffic_signals
