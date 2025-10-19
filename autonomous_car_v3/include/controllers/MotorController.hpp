#pragma once

#include <cstdint>

namespace autonomous_car::controllers {

class MotorController {
public:
    MotorController(int forward_pin_left, int backward_pin_left,
                    int forward_pin_right, int backward_pin_right);

    void forward();
    void backward();
    void stop();

private:
    void setMotorState(int forward_pin, int backward_pin, bool forward_active, bool backward_active);

    int forward_pin_left_;
    int backward_pin_left_;
    int forward_pin_right_;
    int backward_pin_right_;
};

} // namespace autonomous_car::controllers
