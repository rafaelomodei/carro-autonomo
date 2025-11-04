#pragma once

#include "commands/Command.hpp"
#include "controllers/SteeringController.hpp"

namespace autonomous_car::commands {

class CenterSteeringCommand : public Command {
public:
    explicit CenterSteeringCommand(controllers::SteeringController &steering_controller);
    void execute(double value) override;

private:
    controllers::SteeringController &steering_controller_;
};

} // namespace autonomous_car::commands
