#pragma once

#include "commands/Command.hpp"
#include "controllers/MotorController.hpp"

namespace autonomous_car::commands {

class ThrottleCommand : public Command {
public:
    explicit ThrottleCommand(controllers::MotorController &motor_controller);
    void execute(double value) override;

private:
    controllers::MotorController &motor_controller_;
};

} // namespace autonomous_car::commands
