#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "common/DrivingMode.hpp"

namespace autonomous_car::controllers {

enum class CommandRouteStatus { Handled, RejectedByMode, RejectedBySource, UnknownCommand };

class CommandRouter {
public:
    using CommandCallback = std::function<void(std::optional<double>)>;

    void registerCommand(const std::string &name, CommandSource source, CommandCallback callback);

    [[nodiscard]] CommandRouteStatus route(CommandSource source, const std::string &name,
                                           std::optional<double> value,
                                           DrivingMode mode) const;

private:
    struct RegisteredCommand {
        CommandSource source{CommandSource::Manual};
        CommandCallback callback;
    };

    static std::string normalizeName(const std::string &name);

    std::unordered_multimap<std::string, RegisteredCommand> commands_;
};

} // namespace autonomous_car::controllers
