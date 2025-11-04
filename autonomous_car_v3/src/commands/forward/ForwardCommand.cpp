#include "commands/forward/ForwardCommand.hpp"

#include <algorithm>
#include <cmath>

namespace autonomous_car::commands {

ForwardCommand::ForwardCommand(controllers::MotorController &motor_controller)
    : motor_controller_{motor_controller} {}

void ForwardCommand::execute(double intensity) {
    double sanitized = std::abs(intensity);
    if (sanitized <= 0.0) {
        sanitized = 1.0;
    }
    sanitized = std::clamp(sanitized, 0.0, 1.0);
    motor_controller_.forward(sanitized);
}

} // namespace autonomous_car::commands
