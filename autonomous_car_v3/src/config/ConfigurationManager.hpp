#pragma once

#include <mutex>
#include <string>

namespace autonomous_car::config {

struct MotorPinConfig {
    int forward_left{26};
    int backward_left{20};
    int forward_right{19};
    int backward_right{16};
};

struct PidConfig {
    double kp{0.0};
    double ki{0.0};
    double kd{0.0};
    double output_limit{0.0};
    int control_interval_ms{20};
};

struct RuntimeConfigSnapshot {
    MotorPinConfig motor_pins;
    int steering_pwm_pin{13};
    double steering_sensitivity{1.0};
    int steering_center_angle{90};
    int steering_left_limit{20};
    int steering_right_limit{20};
    double steering_command_step{0.1};
    bool motor_left_inverted{false};
    bool motor_right_inverted{true};
    int motor_command_timeout_ms{150};
    PidConfig steering_pid;
};

class ConfigurationManager {
public:
    static ConfigurationManager &instance();

    ConfigurationManager(const ConfigurationManager &) = delete;
    ConfigurationManager &operator=(const ConfigurationManager &) = delete;

    RuntimeConfigSnapshot snapshot() const;

    void loadDefaults();
    bool loadFromFile(const std::string &path);
    bool updateSetting(const std::string &key, const std::string &value);

private:
    ConfigurationManager();

    bool applySetting(const std::string &key, const std::string &value);

    mutable std::mutex mutex_;
    RuntimeConfigSnapshot current_;
};

} // namespace autonomous_car::config
