#include "vision/VisionFrame.hpp"

#include <nlohmann/json.hpp>

#include "vision/Base64.hpp"

namespace {

void setError(std::string *error, const std::string &message) {
    if (error) {
        *error = message;
    }
}

bool isPositiveInt(const nlohmann::json &value) {
    return value.is_number_integer() && value.get<int>() > 0;
}

bool isNonNegativeInteger(const nlohmann::json &value) {
    return value.is_number_integer() && value.get<std::int64_t>() >= 0;
}

} // namespace

namespace traffic_sign_service::vision {

std::optional<VisionFrameMessage> parseVisionFrameMessage(const std::string &payload,
                                                          std::string *error) {
    const auto json = nlohmann::json::parse(payload, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        setError(error, "JSON invalido para vision.frame.");
        return std::nullopt;
    }

    if (json.value("type", "") != "vision.frame") {
        return std::nullopt;
    }

    const auto view = json.value("view", "");
    if (view != "raw") {
        setError(error, "Apenas view raw e suportada pelo servico.");
        return std::nullopt;
    }

    if (!json.contains("timestamp_ms") || !isNonNegativeInteger(json["timestamp_ms"])) {
        setError(error, "timestamp_ms ausente ou invalido.");
        return std::nullopt;
    }

    if (!json.contains("mime") || !json["mime"].is_string()) {
        setError(error, "mime ausente ou invalido.");
        return std::nullopt;
    }

    const auto mime = json["mime"].get<std::string>();
    if (mime.rfind("image/", 0) != 0) {
        setError(error, "mime precisa representar uma imagem.");
        return std::nullopt;
    }

    if (!json.contains("width") || !isPositiveInt(json["width"]) || !json.contains("height") ||
        !isPositiveInt(json["height"])) {
        setError(error, "Dimensoes do frame invalidas.");
        return std::nullopt;
    }

    if (!json.contains("data") || !json["data"].is_string()) {
        setError(error, "data ausente ou invalido.");
        return std::nullopt;
    }

    const auto data = json["data"].get<std::string>();
    if (!isValidBase64(data)) {
        setError(error, "data precisa ser base64 valido.");
        return std::nullopt;
    }

    return VisionFrameMessage{
        view,
        static_cast<std::uint64_t>(json["timestamp_ms"].get<std::int64_t>()),
        mime,
        json["width"].get<int>(),
        json["height"].get<int>(),
        std::move(data),
    };
}

} // namespace traffic_sign_service::vision
