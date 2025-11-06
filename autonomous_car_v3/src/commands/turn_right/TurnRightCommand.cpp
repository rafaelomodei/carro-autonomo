#include "commands/turn_right/TurnRightCommand.hpp"

#include <algorithm>

namespace autonomous_car::commands {

TurnRightCommand::TurnRightCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void TurnRightCommand::execute(double intensity) {
    double sanitized = intensity;
    if (sanitized <= 0.0) {
        sanitized = 1.0;
    }
    sanitized = std::clamp(sanitized, 0.0, 1.0);
    steering_controller_.turnRight(sanitized);
}

} // namespace autonomous_car::commands
