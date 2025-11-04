#pragma once

#include "commands/Command.hpp"
#include "controllers/MotorController.hpp"

namespace autonomous_car::commands {

class BackwardCommand : public Command {
public:
    explicit BackwardCommand(controllers::MotorController &motor_controller);
    void execute(double intensity) override;

private:
    controllers::MotorController &motor_controller_;
};

} // namespace autonomous_car::commands
