#include "commands/turn_right/TurnRightCommand.hpp"

namespace autonomous_car::commands {

TurnRightCommand::TurnRightCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void TurnRightCommand::execute() {
    steering_controller_.turnRight();
}

} // namespace autonomous_car::commands
