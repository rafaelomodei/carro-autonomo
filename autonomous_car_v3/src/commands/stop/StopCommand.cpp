#include "commands/stop/StopCommand.hpp"

namespace autonomous_car::commands {

StopCommand::StopCommand(controllers::MotorController &motor_controller)
    : motor_controller_{motor_controller} {}

void StopCommand::execute(double) {
    motor_controller_.stop();
}

} // namespace autonomous_car::commands
