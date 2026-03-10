#include "controllers/CommandRouter.hpp"

#include <algorithm>
#include <cctype>

namespace autonomous_car::controllers {

std::string CommandRouter::normalizeName(const std::string &name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized;
}

void CommandRouter::registerCommand(const std::string &name, CommandSource source,
                                    CommandCallback callback) {
    commands_.emplace(normalizeName(name), RegisteredCommand{source, std::move(callback)});
}

CommandRouteStatus CommandRouter::route(CommandSource source, const std::string &name,
                                        std::optional<double> value, DrivingMode mode) const {
    if (!isSourceCompatible(source, mode)) {
        return CommandRouteStatus::RejectedByMode;
    }

    const auto normalized_name = normalizeName(name);
    const auto [begin, end] = commands_.equal_range(normalized_name);
    if (begin == end) {
        return CommandRouteStatus::UnknownCommand;
    }

    bool source_match_found = false;
    for (auto it = begin; it != end; ++it) {
        if (it->second.source != source) {
            continue;
        }
        source_match_found = true;
        if (it->second.callback) {
            it->second.callback(value);
            return CommandRouteStatus::Handled;
        }
    }

    return source_match_found ? CommandRouteStatus::UnknownCommand
                              : CommandRouteStatus::RejectedBySource;
}

} // namespace autonomous_car::controllers
