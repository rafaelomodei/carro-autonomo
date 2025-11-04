#pragma once

#include "commands/Command.hpp"
#include "controllers/SteeringController.hpp"

namespace autonomous_car::commands {

class SteeringCommand : public Command {
public:
    explicit SteeringCommand(controllers::SteeringController &steering_controller);
    void execute(double value) override;

private:
    controllers::SteeringController &steering_controller_;
};

} // namespace autonomous_car::commands
