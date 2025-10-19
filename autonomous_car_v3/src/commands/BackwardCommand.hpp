#pragma once

#include "Command.hpp"
#include "controllers/MotorController.hpp"

namespace autonomous_car::commands {

class BackwardCommand : public Command {
public:
    explicit BackwardCommand(controllers::MotorController &motor_controller);
    void execute() override;

private:
    controllers::MotorController &motor_controller_;
};

} // namespace autonomous_car::commands
