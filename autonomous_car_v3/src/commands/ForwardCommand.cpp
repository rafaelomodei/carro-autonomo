#include "commands/ForwardCommand.hpp"

namespace autonomous_car::commands {

ForwardCommand::ForwardCommand(controllers::MotorController &motor_controller)
    : motor_controller_{motor_controller} {}

void ForwardCommand::execute() {
    motor_controller_.forward();
}

} // namespace autonomous_car::commands
