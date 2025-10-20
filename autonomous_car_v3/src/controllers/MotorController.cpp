#include "controllers/MotorController.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <softPwm.h>
#include <wiringPi.h>

namespace autonomous_car::controllers {

namespace {
constexpr int kPwmRange = 100;
}

MotorController::MotorController(int forward_pin_left, int backward_pin_left,
                                 int forward_pin_right, int backward_pin_right)
    : forward_pin_left_{forward_pin_left},
      backward_pin_left_{backward_pin_left},
      forward_pin_right_{forward_pin_right},
      backward_pin_right_{backward_pin_right},
      acceleration_sensitivity_{1.0},
      brake_sensitivity_{1.0} {
    pinMode(forward_pin_left_, OUTPUT);
    pinMode(backward_pin_left_, OUTPUT);
    pinMode(forward_pin_right_, OUTPUT);
    pinMode(backward_pin_right_, OUTPUT);

    if (softPwmCreate(forward_pin_left_, 0, kPwmRange) != 0 ||
        softPwmCreate(backward_pin_left_, 0, kPwmRange) != 0 ||
        softPwmCreate(forward_pin_right_, 0, kPwmRange) != 0 ||
        softPwmCreate(backward_pin_right_, 0, kPwmRange) != 0) {
        std::cerr << "Falha ao inicializar PWM por software nos pinos do motor." << std::endl;
    }

    stop();
}

void MotorController::forward() {
    setMotorPwm(forward_pin_left_, backward_pin_left_, acceleration_sensitivity_, 0.0);
    setMotorPwm(forward_pin_right_, backward_pin_right_, acceleration_sensitivity_, 0.0);
}

void MotorController::backward() {
    setMotorPwm(forward_pin_left_, backward_pin_left_, 0.0, brake_sensitivity_);
    setMotorPwm(forward_pin_right_, backward_pin_right_, 0.0, brake_sensitivity_);
}

void MotorController::stop() {
    setMotorPwm(forward_pin_left_, backward_pin_left_, 0.0, 0.0);
    setMotorPwm(forward_pin_right_, backward_pin_right_, 0.0, 0.0);
}

void MotorController::setAccelerationSensitivity(double sensitivity) {
    acceleration_sensitivity_ = clampIntensity(sensitivity);
}

void MotorController::setBrakeSensitivity(double sensitivity) {
    brake_sensitivity_ = clampIntensity(sensitivity);
}

void MotorController::setMotorPwm(int forward_pin, int backward_pin, double forward_intensity, double backward_intensity) {
    double clamped_forward = clampIntensity(forward_intensity);
    double clamped_backward = clampIntensity(backward_intensity);

    softPwmWrite(forward_pin, toDutyCycle(clamped_forward));
    softPwmWrite(backward_pin, toDutyCycle(clamped_backward));
}

double MotorController::clampIntensity(double intensity) {
    if (std::isnan(intensity) || std::isinf(intensity)) {
        return 0.0;
    }
    return std::clamp(intensity, 0.0, 1.0);
}

int MotorController::toDutyCycle(double intensity) {
    return static_cast<int>(std::round(intensity * kPwmRange));
}

} // namespace autonomous_car::controllers
