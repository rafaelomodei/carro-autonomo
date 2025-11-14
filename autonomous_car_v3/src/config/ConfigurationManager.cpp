#include "config/ConfigurationManager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

namespace {

std::string trim(const std::string &value) {
    auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool iequals(const std::string &lhs, const std::string &rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<int> parseInt(const std::string &value) {
    try {
        size_t consumed = 0;
        int result = std::stoi(value, &consumed, 10);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<double> parseDouble(const std::string &value) {
    try {
        size_t consumed = 0;
        double result = std::stod(value, &consumed);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<bool> parseBool(const std::string &value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (char ch : value) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
        return true;
    }
    if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
        return false;
    }
    return std::nullopt;
}

} // namespace

namespace autonomous_car::config {

ConfigurationManager &ConfigurationManager::instance() {
    static ConfigurationManager instance;
    return instance;
}

ConfigurationManager::ConfigurationManager() { loadDefaults(); }

RuntimeConfigSnapshot ConfigurationManager::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_;
}

void ConfigurationManager::loadDefaults() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_ = RuntimeConfigSnapshot{};
    current_.steering_center_angle = 90;
    current_.steering_left_limit = 20;
    current_.steering_right_limit = 20;
    current_.steering_command_step = 0.1;
    current_.motor_command_timeout_ms = 150;
    current_.driving_mode = DrivingMode::Manual;
}

bool ConfigurationManager::loadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Não foi possível abrir o arquivo de configuração: " << path << std::endl;
        return false;
    }

    bool all_success = true;
    std::string line;
    while (std::getline(file, line)) {
        auto trimmed_line = trim(line);
        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue;
        }

        auto delimiter_pos = trimmed_line.find('=');
        if (delimiter_pos == std::string::npos) {
            std::cerr << "Linha de configuração inválida: " << trimmed_line << std::endl;
            all_success = false;
            continue;
        }

        auto key = trim(trimmed_line.substr(0, delimiter_pos));
        auto value = trim(trimmed_line.substr(delimiter_pos + 1));
        if (!applySetting(key, value)) {
            std::cerr << "Chave de configuração desconhecida ou valor inválido: " << key << std::endl;
            all_success = false;
        }
    }

    return all_success;
}

bool ConfigurationManager::updateSetting(const std::string &key, const std::string &value) {
    if (!applySetting(key, value)) {
        return false;
    }
    std::cout << "Configuração atualizada: " << key << "=" << value << std::endl;
    return true;
}

bool ConfigurationManager::applySetting(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (iequals(key, "MOTOR_FORWARD_LEFT_PIN") || iequals(key, "motor.forward_left_pin")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        current_.motor_pins.forward_left = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_BACKWARD_LEFT_PIN") || iequals(key, "motor.backward_left_pin")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        current_.motor_pins.backward_left = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_FORWARD_RIGHT_PIN") || iequals(key, "motor.forward_right_pin")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        current_.motor_pins.forward_right = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_BACKWARD_RIGHT_PIN") || iequals(key, "motor.backward_right_pin")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        current_.motor_pins.backward_right = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_PWM_PIN") || iequals(key, "steering.pwm_pin")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        current_.steering_pwm_pin = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_SENSITIVITY") || iequals(key, "steering.sensitivity")) {
        auto parsed = parseDouble(value);
        if (!parsed || *parsed <= 0.0) {
            return false;
        }
        current_.steering_sensitivity = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_COMMAND_STEP") || iequals(key, "steering.command_step")) {
        auto parsed = parseDouble(value);
        if (!parsed || *parsed <= 0.0) {
            return false;
        }
        current_.steering_command_step = std::min(*parsed, 1.0);
        return true;
    }

    if (iequals(key, "STEERING_CENTER_ANGLE") || iequals(key, "steering.center_angle")) {
        auto parsed = parseInt(value);
        if (!parsed) {
            return false;
        }
        if (*parsed < 0 || *parsed > 180) {
            return false;
        }
        current_.steering_center_angle = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_LEFT_LIMIT_DEGREES") || iequals(key, "steering.left_limit_degrees")) {
        auto parsed = parseInt(value);
        if (!parsed || *parsed < 0) {
            return false;
        }
        current_.steering_left_limit = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_RIGHT_LIMIT_DEGREES") || iequals(key, "steering.right_limit_degrees")) {
        auto parsed = parseInt(value);
        if (!parsed || *parsed < 0) {
            return false;
        }
        current_.steering_right_limit = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_LEFT_INVERTED") || iequals(key, "motor.left_inverted")) {
        auto parsed = parseBool(value);
        if (!parsed) {
            return false;
        }
        current_.motor_left_inverted = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_RIGHT_INVERTED") || iequals(key, "motor.right_inverted")) {
        auto parsed = parseBool(value);
        if (!parsed) {
            return false;
        }
        current_.motor_right_inverted = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_COMMAND_TIMEOUT_MS") || iequals(key, "motor.command_timeout_ms")) {
        auto parsed = parseInt(value);
        if (!parsed || *parsed < 0) {
            return false;
        }
        current_.motor_command_timeout_ms = *parsed;
        return true;
    }

    if (iequals(key, "MOTOR_PID_KP") || iequals(key, "motor.pid.kp") ||
        iequals(key, "MOTOR_PID_KI") || iequals(key, "motor.pid.ki") ||
        iequals(key, "MOTOR_PID_KD") || iequals(key, "motor.pid.kd") ||
        iequals(key, "MOTOR_PID_OUTPUT_LIMIT") || iequals(key, "motor.pid.output_limit") ||
        iequals(key, "MOTOR_PID_INTERVAL_MS") || iequals(key, "motor.pid.interval_ms") ||
        iequals(key, "MOTOR_MIN_ACTIVE_THROTTLE") ||
        iequals(key, "motor.min_active_throttle")) {
        std::cerr << "Configuração de PID do motor obsoleta ignorada: " << key << std::endl;
        return true;
    }

    if (iequals(key, "DRIVING_MODE") || iequals(key, "driving.mode")) {
        auto parsed = drivingModeFromString(value);
        if (!parsed) {
            return false;
        }
        current_.driving_mode = *parsed;
        return true;
    }

    if (iequals(key, "STEERING_PID_KP") || iequals(key, "steering.pid.kp") ||
        iequals(key, "STEERING_PID_KI") || iequals(key, "steering.pid.ki") ||
        iequals(key, "STEERING_PID_KD") || iequals(key, "steering.pid.kd") ||
        iequals(key, "STEERING_PID_OUTPUT_LIMIT") || iequals(key, "steering.pid.output_limit") ||
        iequals(key, "STEERING_PID_INTERVAL_MS") || iequals(key, "steering.pid.interval_ms")) {
        std::cerr << "Configuração de PID da direção obsoleta ignorada: " << key << std::endl;
        return true;
    }

    return false;
}

} // namespace autonomous_car::config
