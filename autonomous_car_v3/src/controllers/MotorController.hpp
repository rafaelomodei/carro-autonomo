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
    void setAccelerationSensitivity(double sensitivity);
    void setBrakeSensitivity(double sensitivity);

private:
    void setMotorPwm(int forward_pin, int backward_pin, double forward_intensity, double backward_intensity);
    static double clampIntensity(double intensity);
    static int toDutyCycle(double intensity);

    int forward_pin_left_;
    int backward_pin_left_;
    int forward_pin_right_;
    int backward_pin_right_;
    double acceleration_sensitivity_;
    double brake_sensitivity_;
};

} // namespace autonomous_car::controllers
