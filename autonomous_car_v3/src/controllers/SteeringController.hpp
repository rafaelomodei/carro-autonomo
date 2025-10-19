#pragma once

namespace autonomous_car::controllers {

class SteeringController {
public:
    SteeringController(int pwm_pin, int min_angle = 0, int max_angle = 180);

    void turnLeft();
    void turnRight();
    void center();
    void setAngle(int angle);

private:
    int clampAngle(int angle) const;
    int toPwmValue(int angle) const;

    int pwm_pin_;
    int min_angle_;
    int max_angle_;
    int center_angle_;
    int min_pwm_;
    int max_pwm_;
};

} // namespace autonomous_car::controllers
