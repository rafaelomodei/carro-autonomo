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

    if (iequals(key, "ACCELERATION_SENSITIVITY") || iequals(key, "drive.acceleration_sensitivity")) {
        auto parsed = parseDouble(value);
        if (!parsed || *parsed <= 0.0) {
            return false;
        }
        current_.acceleration_sensitivity = *parsed;
        return true;
    }

    if (iequals(key, "BRAKE_SENSITIVITY") || iequals(key, "drive.brake_sensitivity")) {
        auto parsed = parseDouble(value);
        if (!parsed || *parsed <= 0.0) {
            return false;
        }
        current_.brake_sensitivity = *parsed;
        return true;
    }

    return false;
}

} // namespace autonomous_car::config
