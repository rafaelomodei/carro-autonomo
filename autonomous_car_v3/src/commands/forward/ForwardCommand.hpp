#pragma once

#include "commands/Command.hpp"
#include "controllers/MotorController.hpp"

namespace autonomous_car::commands {

class ForwardCommand : public Command {
public:
    explicit ForwardCommand(controllers::MotorController &motor_controller);
    void execute() override;

private:
    controllers::MotorController &motor_controller_;
};

} // namespace autonomous_car::commands
