#pragma once

#include <mutex>
#include <string>

namespace autonomous_car::config {

struct MotorPinConfig {
    int forward_left{17};
    int backward_left{27};
    int forward_right{23};
    int backward_right{22};
};

struct RuntimeConfigSnapshot {
    MotorPinConfig motor_pins;
    int steering_pwm_pin{18};
    double steering_sensitivity{1.0};
    double acceleration_sensitivity{1.0};
    double brake_sensitivity{1.0};
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
