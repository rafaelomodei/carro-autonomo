#include "commands/BackwardCommand.hpp"

namespace autonomous_car::commands {

BackwardCommand::BackwardCommand(controllers::MotorController &motor_controller)
    : motor_controller_{motor_controller} {}

void BackwardCommand::execute() {
    motor_controller_.backward();
}

} // namespace autonomous_car::commands
