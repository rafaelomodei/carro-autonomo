#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "controllers/PidController.hpp"

namespace autonomous_car::controllers {

class SteeringController {
public:
    struct DynamicsConfig {
        double kp{3.0};
        double ki{0.0};
        double kd{0.45};
        double output_limit{0.2};
        int control_interval_ms{20};
    };

    SteeringController(int pwm_pin, int min_angle = 0, int max_angle = 180);
    ~SteeringController();

    void turnLeft(double intensity = 1.0);
    void turnRight(double intensity = 1.0);
    void center();
    void setSteering(double normalized_value);
    void setAngle(int angle);

    void setSteeringSensitivity(double sensitivity);
    void setDynamics(const DynamicsConfig &config);

private:
    void initializePwm();
    void controlLoop();
    void applyAngle(int angle);
    int clampAngle(int angle) const;
    int toPwmValue(int angle) const;
    int steeringDelta() const;

    int pwm_pin_;
    int min_angle_;
    int max_angle_;
    int center_angle_;
    int min_pwm_;
    int max_pwm_;
    std::atomic<double> steering_sensitivity_;

    PidController pid_;
    std::thread control_thread_;
    std::atomic<bool> running_;
    std::atomic<int> control_interval_ms_;

    mutable std::mutex state_mutex_;
    double target_offset_;
    double current_offset_;
};

} // namespace autonomous_car::controllers
