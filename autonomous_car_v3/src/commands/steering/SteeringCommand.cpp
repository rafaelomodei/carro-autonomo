#include "commands/steering/SteeringCommand.hpp"

#include <algorithm>

namespace autonomous_car::commands {

SteeringCommand::SteeringCommand(controllers::SteeringController &steering_controller)
    : steering_controller_{steering_controller} {}

void SteeringCommand::execute(double value) {
    double clamped = std::clamp(value, -1.0, 1.0);
    steering_controller_.setSteering(clamped);
}

} // namespace autonomous_car::commands
