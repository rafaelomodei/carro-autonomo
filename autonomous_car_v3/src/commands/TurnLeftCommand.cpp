#include "commands/TurnLeftCommand.hpp"

namespace autonomous_car::commands {

TurnLeftCommand::TurnLeftCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void TurnLeftCommand::execute() {
    steering_controller_.turnLeft();
}

} // namespace autonomous_car::commands
