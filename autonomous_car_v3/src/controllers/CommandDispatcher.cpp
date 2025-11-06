#include "controllers/CommandDispatcher.hpp"

namespace autonomous_car::controllers {

void CommandDispatcher::registerCommand(const std::string &name, std::unique_ptr<commands::Command> command) {
    commands_[name] = std::move(command);
}

bool CommandDispatcher::dispatch(const std::string &name, std::optional<double> value) {
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        return false;
    }

    double parameter = value.value_or(0.0);
    it->second->execute(parameter);
    return true;
}

} // namespace autonomous_car::controllers
