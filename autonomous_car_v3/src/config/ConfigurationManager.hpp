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

struct RuntimeConfigSnapshot {
    MotorPinConfig motor_pins;
    int steering_pwm_pin{13};
    double steering_sensitivity{1.0};
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
