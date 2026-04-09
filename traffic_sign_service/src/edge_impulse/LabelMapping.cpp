#include "edge_impulse/LabelMapping.hpp"

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

namespace traffic_sign_service::edge_impulse {

std::optional<TrafficSignalId> trafficSignalIdFromModelLabel(std::string_view label) {
    const auto normalized = normalize(label);

    if (normalized == "parada obrigatoria sign") {
        return TrafficSignalId::Stop;
    }

    if (normalized == "vire a esquerda sign") {
        return TrafficSignalId::TurnLeft;
    }

    return std::nullopt;
}

} // namespace traffic_sign_service::edge_impulse
