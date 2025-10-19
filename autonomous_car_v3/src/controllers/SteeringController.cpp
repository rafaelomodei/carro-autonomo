#include "controllers/SteeringController.hpp"

#include <algorithm>
#include <iostream>

#include <softPwm.h>

namespace autonomous_car::controllers {

namespace {
constexpr int kDefaultMinPwm = 5;   // ~0.5ms pulse width
constexpr int kDefaultMaxPwm = 25;  // ~2.5ms pulse width
constexpr int kLeftAngle = 45;
constexpr int kRightAngle = 135;
constexpr int kCenterAngle = 90;
}

SteeringController::SteeringController(int pwm_pin, int min_angle, int max_angle)
    : pwm_pin_{pwm_pin},
      min_angle_{min_angle},
      max_angle_{max_angle},
      center_angle_{kCenterAngle},
      min_pwm_{kDefaultMinPwm},
      max_pwm_{kDefaultMaxPwm} {
    if (softPwmCreate(pwm_pin_, 0, 200) != 0) {
        std::cerr << "Falha ao iniciar PWM por software no pino " << pwm_pin_ << std::endl;
    }
    center();
}

void SteeringController::turnLeft() {
    setAngle(kLeftAngle);
}

void SteeringController::turnRight() {
    setAngle(kRightAngle);
}

void SteeringController::center() {
    setAngle(center_angle_);
}

void SteeringController::setAngle(int angle) {
    int clamped = clampAngle(angle);
    int pwm_value = toPwmValue(clamped);
    softPwmWrite(pwm_pin_, pwm_value);
}

int SteeringController::clampAngle(int angle) const {
    return std::clamp(angle, min_angle_, max_angle_);
}

int SteeringController::toPwmValue(int angle) const {
    if (max_angle_ == min_angle_) {
        return min_pwm_;
    }
    double proportion = static_cast<double>(angle - min_angle_) /
                         static_cast<double>(max_angle_ - min_angle_);
    int range = max_pwm_ - min_pwm_;
    return min_pwm_ + static_cast<int>(proportion * range);
}

} // namespace autonomous_car::controllers
