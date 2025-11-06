#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "controllers/PidController.hpp"

namespace autonomous_car::controllers {

class MotorController {
public:
    struct DynamicsConfig {
        bool invert_left{false};
        bool invert_right{true};
        double kp{2.5};
        double ki{0.0};
        double kd{0.35};
        double output_limit{0.15};
        int control_interval_ms{20};
        double min_active_throttle{0.2};
    };

    MotorController(int forward_pin_left, int backward_pin_left,
                    int forward_pin_right, int backward_pin_right);
    ~MotorController();

    void forward(double intensity = 1.0);
    void backward(double intensity = 1.0);
    void stop();
    void setThrottle(double normalized_value);

    void setDynamics(const DynamicsConfig &config);

private:
    void initializePins();
    void controlLoop();
    void applyThrottle(double throttle);
    void applyMotor(int forward_pin, int backward_pin, double throttle, bool invert);

    int forward_pin_left_;
    int backward_pin_left_;
    int forward_pin_right_;
    int backward_pin_right_;

    bool invert_left_;
    bool invert_right_;

    PidController pid_;
    std::thread control_thread_;
    std::atomic<bool> running_;
    std::atomic<int> control_interval_ms_;
    std::atomic<double> max_delta_per_interval_;
    std::atomic<double> min_active_throttle_;

    mutable std::mutex state_mutex_;
    double target_throttle_;
    double current_throttle_;
};

} // namespace autonomous_car::controllers
