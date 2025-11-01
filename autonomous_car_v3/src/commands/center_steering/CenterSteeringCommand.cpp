#include "commands/center_steering/CenterSteeringCommand.hpp"

namespace autonomous_car::commands {

CenterSteeringCommand::CenterSteeringCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void CenterSteeringCommand::execute() {
    steering_controller_.center();
}

} // namespace autonomous_car::commands
