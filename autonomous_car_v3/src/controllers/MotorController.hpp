#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace autonomous_car::controllers {

class MotorController {
public:
    struct DynamicsConfig {
        bool invert_left{false};
        bool invert_right{true};
        int command_timeout_ms{150};
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
    enum class Motion { Stopped, Forward, Backward };

    void initializePins();
    void controlLoop();
    void setCommand(Motion motion);
    void applyMotion(Motion motion);
    void applyMotor(int forward_pin, int backward_pin, Motion motion, bool invert);

    int forward_pin_left_;
    int backward_pin_left_;
    int forward_pin_right_;
    int backward_pin_right_;

    bool invert_left_;
    bool invert_right_;

    std::thread control_thread_;
    std::atomic<bool> running_;
    std::atomic<int> command_timeout_ms_;

    mutable std::mutex state_mutex_;
    Motion requested_motion_;
    Motion applied_motion_;
    bool command_active_;
    std::chrono::steady_clock::time_point last_command_time_;
};

} // namespace autonomous_car::controllers
