#include "domain/TrafficSignal.hpp"

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

namespace traffic_sign_service {

std::string_view toString(TrafficSignalId signal_id) {
    switch (signal_id) {
    case TrafficSignalId::Stop:
        return "stop";
    case TrafficSignalId::TurnLeft:
        return "turn_left";
    case TrafficSignalId::TurnRight:
        return "turn_right";
    case TrafficSignalId::Unknown:
        return "unknown";
    }
    return "unknown";
}

std::optional<TrafficSignalId> trafficSignalIdFromCanonicalString(std::string_view value) {
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
    if (normalized == "unknown") {
        return TrafficSignalId::Unknown;
    }
    return std::nullopt;
}

std::string displayLabel(TrafficSignalId signal_id) {
    switch (signal_id) {
    case TrafficSignalId::Stop:
        return "Parada obrigatoria";
    case TrafficSignalId::TurnLeft:
        return "Vire a esquerda";
    case TrafficSignalId::TurnRight:
        return "Vire a direita";
    case TrafficSignalId::Unknown:
        return "Sinal desconhecido";
    }

    return "Sinal desconhecido";
}

} // namespace traffic_sign_service
