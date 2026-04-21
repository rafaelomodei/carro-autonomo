#pragma once

#include <optional>
#include <string>

#include "services/websocket/ClientRegistry.hpp"

namespace autonomous_car::services::websocket {

enum class MessageChannel { Client, Command, Config, Stream, Signal, Telemetry, Unknown };

struct ParsedMessage {
    MessageChannel channel{MessageChannel::Unknown};
    std::string key;
    std::optional<std::string> value;
    std::optional<std::string> source_token;
};

std::optional<ParsedMessage> parseInboundMessage(const std::string &payload);
std::optional<ClientRole> parseClientRole(const std::string &value);
std::optional<double> parseCommandValue(const std::optional<std::string> &raw_value);
bool messageRequiresControllerRole(MessageChannel channel);

} // namespace autonomous_car::services::websocket
