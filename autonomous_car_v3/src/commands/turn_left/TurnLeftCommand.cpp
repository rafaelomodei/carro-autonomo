#include "commands/turn_left/TurnLeftCommand.hpp"

#include <algorithm>
#include <cmath>

namespace autonomous_car::commands {

TurnLeftCommand::TurnLeftCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void TurnLeftCommand::execute(double intensity) {
    double sanitized = std::abs(intensity);
    if (sanitized <= 0.0) {
        sanitized = 1.0;
    }
    sanitized = std::clamp(sanitized, 0.0, 1.0);
    steering_controller_.turnLeft(sanitized);
}

} // namespace autonomous_car::commands
