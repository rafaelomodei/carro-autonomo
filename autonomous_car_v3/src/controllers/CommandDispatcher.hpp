#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "commands/Command.hpp"

namespace autonomous_car::controllers {

class CommandDispatcher {
public:
    void registerCommand(const std::string &name, std::unique_ptr<commands::Command> command);
    bool dispatch(const std::string &name);

private:
    std::unordered_map<std::string, std::unique_ptr<commands::Command>> commands_;
};

} // namespace autonomous_car::controllers
