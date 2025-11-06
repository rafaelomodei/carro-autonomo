#include "commands/throttle/ThrottleCommand.hpp"

#include <algorithm>

namespace autonomous_car::commands {

ThrottleCommand::ThrottleCommand(controllers::MotorController &motor_controller)
    : motor_controller_{motor_controller} {}

void ThrottleCommand::execute(double value) {
    double clamped = std::clamp(value, -1.0, 1.0);
    motor_controller_.setThrottle(clamped);
}

} // namespace autonomous_car::commands
