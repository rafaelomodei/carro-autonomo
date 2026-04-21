#include "edge_impulse/LabelMapping.hpp"

#include <string>

namespace traffic_sign_service::edge_impulse {

TrafficSignalId trafficSignalIdFromModelLabel(std::string_view label) {
    std::string normalized;
    normalized.reserve(label.size());

    for (char ch : label) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    if (normalized == "paradaobrigatoriasign" || normalized == "stopsign") {
        return TrafficSignalId::Stop;
    }

    if (normalized == "vireaesquerdasign" || normalized == "turnleftsign" ||
        normalized == "turnleft") {
        return TrafficSignalId::TurnLeft;
    }

    if (normalized == "vireadireitasign" || normalized == "turnrightsign" ||
        normalized == "turnright") {
        return TrafficSignalId::TurnRight;
    }

    return TrafficSignalId::Unknown;
}

} // namespace traffic_sign_service::edge_impulse
