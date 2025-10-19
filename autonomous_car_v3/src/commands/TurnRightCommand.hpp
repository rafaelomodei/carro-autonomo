#pragma once

#include "Command.hpp"
#include "controllers/SteeringController.hpp"

namespace autonomous_car::commands {

class TurnRightCommand : public Command {
public:
    explicit TurnRightCommand(controllers::SteeringController &steering_controller);
    void execute() override;

private:
    controllers::SteeringController &steering_controller_;
};

} // namespace autonomous_car::commands
