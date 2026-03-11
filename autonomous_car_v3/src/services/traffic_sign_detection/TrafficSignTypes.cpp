#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace autonomous_car::services::traffic_sign_detection {
namespace {

std::string normalizeToken(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (const char ch : value) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    return normalized;
}

} // namespace

std::string_view toString(TrafficSignId id) {
    switch (id) {
    case TrafficSignId::Stop:
        return "stop";
    case TrafficSignId::TurnLeft:
        return "turn_left";
    case TrafficSignId::TurnRight:
        return "turn_right";
    }

    return "stop";
}

std::string_view toString(TrafficSignDetectorState state) {
    switch (state) {
    case TrafficSignDetectorState::Disabled:
        return "disabled";
    case TrafficSignDetectorState::Idle:
        return "idle";
    case TrafficSignDetectorState::Candidate:
        return "candidate";
    case TrafficSignDetectorState::Confirmed:
        return "confirmed";
    case TrafficSignDetectorState::Error:
        return "error";
    }

    return "idle";
}

std::string displayLabel(TrafficSignId id) {
    switch (id) {
    case TrafficSignId::Stop:
        return "Parada obrigatoria";
    case TrafficSignId::TurnLeft:
        return "Vire a esquerda";
    case TrafficSignId::TurnRight:
        return "Vire a direita";
    }

    return "Sinal desconhecido";
}

std::optional<TrafficSignId> trafficSignIdFromModelLabel(std::string_view label) {
    const std::string normalized = normalizeToken(label);
    if (normalized == "paradaobrigatoriasign" || normalized == "stopsign" ||
        normalized == "stop") {
        return TrafficSignId::Stop;
    }

    if (normalized == "vireaesquerdasign" || normalized == "turnleftsign" ||
        normalized == "turnleft") {
        return TrafficSignId::TurnLeft;
    }

    if (normalized == "vireadireitasign" || normalized == "turnrightsign" ||
        normalized == "turnright") {
        return TrafficSignId::TurnRight;
    }

    return std::nullopt;
}

std::string buildModelCompatibilityWarning(const std::vector<std::string> &model_categories) {
    bool has_stop = false;
    bool has_left = false;
    bool has_right = false;

    for (const auto &category : model_categories) {
        const auto id = trafficSignIdFromModelLabel(category);
        if (!id.has_value()) {
            continue;
        }

        switch (*id) {
        case TrafficSignId::Stop:
            has_stop = true;
            break;
        case TrafficSignId::TurnLeft:
            has_left = true;
            break;
        case TrafficSignId::TurnRight:
            has_right = true;
            break;
        }
    }

    std::vector<std::string> missing;
    if (!has_stop) {
        missing.emplace_back("stop");
    }
    if (!has_left) {
        missing.emplace_back("turn_left");
    }
    if (!has_right) {
        missing.emplace_back("turn_right");
    }

    if (missing.empty()) {
        return {};
    }

    std::ostringstream stream;
    stream << "Modelo Edge Impulse sem classes esperadas: ";
    for (std::size_t index = 0; index < missing.size(); ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << missing[index];
    }
    stream << ". Substitua o export pelo pacote atualizado do V3.";
    return stream.str();
}

cv::Rect toCvRect(const DetectionBox &box) {
    return {box.x, box.y, std::max(0, box.width), std::max(0, box.height)};
}

double detectionArea(const TrafficSignDetection &detection) {
    return static_cast<double>(std::max(0, detection.bbox_frame.width)) *
           static_cast<double>(std::max(0, detection.bbox_frame.height));
}

bool isBetterDetection(const TrafficSignDetection &lhs, const TrafficSignDetection &rhs) {
    if (lhs.confidence_score == rhs.confidence_score) {
        return detectionArea(lhs) > detectionArea(rhs);
    }

    return lhs.confidence_score > rhs.confidence_score;
}

} // namespace autonomous_car::services::traffic_sign_detection
