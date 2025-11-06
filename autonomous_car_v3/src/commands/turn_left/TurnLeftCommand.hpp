#pragma once

#include "commands/Command.hpp"
#include "controllers/SteeringController.hpp"

namespace autonomous_car::commands {

class TurnLeftCommand : public Command {
public:
    explicit TurnLeftCommand(controllers::SteeringController &steering_controller);
    void execute(double intensity) override;

private:
    controllers::SteeringController &steering_controller_;
};

} // namespace autonomous_car::commands
