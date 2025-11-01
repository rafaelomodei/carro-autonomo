#include "controllers/MotorController.hpp"

#include <wiringPi.h>

namespace autonomous_car::controllers {

MotorController::MotorController(int forward_pin_left, int backward_pin_left,
                                 int forward_pin_right, int backward_pin_right)
    : forward_pin_left_{forward_pin_left},
      backward_pin_left_{backward_pin_left},
      forward_pin_right_{forward_pin_right},
      backward_pin_right_{backward_pin_right} {
    pinMode(forward_pin_left_, OUTPUT);
    pinMode(backward_pin_left_, OUTPUT);
    pinMode(forward_pin_right_, OUTPUT);
    pinMode(backward_pin_right_, OUTPUT);

    stop();
}

void MotorController::forward() {
    setMotorState(forward_pin_left_, backward_pin_left_, true, false);
    setMotorState(forward_pin_right_, backward_pin_right_, true, false);
}

void MotorController::backward() {
    setMotorState(forward_pin_left_, backward_pin_left_, false, true);
    setMotorState(forward_pin_right_, backward_pin_right_, false, true);
}

void MotorController::stop() {
    setMotorState(forward_pin_left_, backward_pin_left_, false, false);
    setMotorState(forward_pin_right_, backward_pin_right_, false, false);
}

void MotorController::setMotorState(int forward_pin, int backward_pin, bool forward_active, bool backward_active) {
    digitalWrite(forward_pin, forward_active ? HIGH : LOW);
    digitalWrite(backward_pin, backward_active ? HIGH : LOW);
}

} // namespace autonomous_car::controllers
