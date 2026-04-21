#include "services/websocket/WebSocketProtocol.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>

namespace {

std::string trim(const std::string &value) {
    auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool isTrafficSignTelemetryJson(std::string_view payload) {
    const auto type_key = payload.find("\"type\"");
    if (type_key == std::string_view::npos) {
        return false;
    }

    return payload.find("\"telemetry.traffic_sign_detection\"", type_key) !=
           std::string_view::npos;
}

} // namespace

namespace autonomous_car::services::websocket {

std::optional<ParsedMessage> parseInboundMessage(const std::string &payload) {
    auto trimmed = trim(payload);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    if (!trimmed.empty() && trimmed.front() == '{' && isTrafficSignTelemetryJson(trimmed)) {
        ParsedMessage parsed;
        parsed.channel = MessageChannel::Telemetry;
        parsed.key = "telemetry.traffic_sign_detection";
        parsed.value = trimmed;
        return parsed;
    }

    auto delimiter_pos = trimmed.find(':');
    if (delimiter_pos == std::string::npos) {
        ParsedMessage parsed;
        parsed.channel = MessageChannel::Command;
        parsed.key = trimmed;
        return parsed;
    }

    auto channel_token = toLower(trim(trimmed.substr(0, delimiter_pos)));
    auto remainder = trim(trimmed.substr(delimiter_pos + 1));

    if (channel_token == "client") {
        ParsedMessage parsed;
        parsed.channel = MessageChannel::Client;
        parsed.key = toLower(remainder);
        return parsed;
    }

    if (channel_token == "command") {
        ParsedMessage parsed;
        parsed.channel = MessageChannel::Command;
        auto equals_pos = remainder.find('=');
        std::string command_section;
        if (equals_pos == std::string::npos) {
            command_section = remainder;
        } else {
            command_section = trim(remainder.substr(0, equals_pos));
            parsed.value = trim(remainder.substr(equals_pos + 1));
            if (parsed.value && parsed.value->empty()) {
                parsed.value = std::nullopt;
            }
        }

        auto colon_pos = command_section.find(':');
        if (colon_pos != std::string::npos) {
            auto possible_source = trim(command_section.substr(0, colon_pos));
            auto remainder_section = trim(command_section.substr(colon_pos + 1));
            if (!possible_source.empty() && !remainder_section.empty()) {
                parsed.source_token = possible_source;
                command_section = remainder_section;
            }
        }

        if (command_section.empty()) {
            return std::nullopt;
        }

        parsed.key = command_section;
        return parsed;
    }

    if (channel_token == "config") {
        auto equals_pos = remainder.find('=');
        if (equals_pos == std::string::npos) {
            return std::nullopt;
        }

        ParsedMessage parsed;
        parsed.channel = MessageChannel::Config;
        parsed.key = trim(remainder.substr(0, equals_pos));
        parsed.value = trim(remainder.substr(equals_pos + 1));
        return parsed;
    }

    if (channel_token == "stream") {
        const auto equals_pos = remainder.find('=');
        if (equals_pos == std::string::npos) {
            return std::nullopt;
        }

        ParsedMessage parsed;
        parsed.channel = MessageChannel::Stream;
        parsed.key = trim(remainder.substr(0, equals_pos));
        parsed.value = trim(remainder.substr(equals_pos + 1));
        return parsed;
    }

    if (channel_token == "signal") {
        const auto equals_pos = remainder.find('=');
        if (equals_pos == std::string::npos) {
            return std::nullopt;
        }

        ParsedMessage parsed;
        parsed.channel = MessageChannel::Signal;
        parsed.key = trim(remainder.substr(0, equals_pos));
        parsed.value = trim(remainder.substr(equals_pos + 1));
        if (parsed.key.empty() || !parsed.value || parsed.value->empty()) {
            return std::nullopt;
        }
        return parsed;
    }

    return std::nullopt;
}

std::optional<ClientRole> parseClientRole(const std::string &value) {
    if (value == "control") {
        return ClientRole::Control;
    }
    if (value == "telemetry") {
        return ClientRole::Telemetry;
    }
    return std::nullopt;
}

std::optional<double> parseCommandValue(const std::optional<std::string> &raw_value) {
    if (!raw_value || raw_value->empty()) {
        return std::nullopt;
    }

    std::string sanitized = trim(*raw_value);
    if (!sanitized.empty() && sanitized.back() == '%') {
        sanitized.pop_back();
    }

    try {
        double parsed = std::stod(sanitized);
        if (!std::isfinite(parsed)) {
            return std::nullopt;
        }
        if (std::abs(parsed) > 1.0) {
            parsed /= 100.0;
        }
        parsed = std::clamp(parsed, -1.0, 1.0);
        return parsed;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

bool messageRequiresControllerRole(MessageChannel channel) {
    return channel == MessageChannel::Command || channel == MessageChannel::Config;
}

} // namespace autonomous_car::services::websocket
