#include "controllers/MotorController.hpp"

#include <algorithm>
#include <cmath>

#include <wiringPi.h>

namespace autonomous_car::controllers {

namespace {
constexpr double kMinNormalizedThrottle = -1.0;
constexpr double kMaxNormalizedThrottle = 1.0;
constexpr int kDefaultCommandTimeoutMs = 150;
constexpr auto kControlInterval = std::chrono::milliseconds(10);
} // namespace

MotorController::MotorController(int forward_pin_left, int backward_pin_left,
                                 int forward_pin_right, int backward_pin_right)
    : forward_pin_left_{forward_pin_left},
      backward_pin_left_{backward_pin_left},
      forward_pin_right_{forward_pin_right},
      backward_pin_right_{backward_pin_right},
      invert_left_{false},
      invert_right_{true},
      running_{true},
      command_timeout_ms_{kDefaultCommandTimeoutMs},
      requested_motion_{Motion::Stopped},
      applied_motion_{Motion::Stopped},
      command_active_{false},
      last_command_time_{std::chrono::steady_clock::now()} {
    initializePins();

    control_thread_ = std::thread(&MotorController::controlLoop, this);
}

MotorController::~MotorController() {
    running_.store(false);
    if (control_thread_.joinable()) {
        control_thread_.join();
    }

    applyMotion(Motion::Stopped);
    digitalWrite(forward_pin_left_, LOW);
    digitalWrite(backward_pin_left_, LOW);
    digitalWrite(forward_pin_right_, LOW);
    digitalWrite(backward_pin_right_, LOW);
}

void MotorController::forward(double intensity) {
    double normalized = std::clamp(intensity, 0.0, 1.0);
    if (normalized <= 0.0) {
        stop();
        return;
    }
    setCommand(Motion::Forward);
}

void MotorController::backward(double intensity) {
    double normalized = std::clamp(std::abs(intensity), 0.0, 1.0);
    if (normalized <= 0.0) {
        stop();
        return;
    }
    setCommand(Motion::Backward);
}

void MotorController::stop() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    requested_motion_ = Motion::Stopped;
    command_active_ = false;
}

void MotorController::setThrottle(double normalized_value) {
    double clamped = std::clamp(normalized_value, kMinNormalizedThrottle, kMaxNormalizedThrottle);
    if (clamped > 0.0) {
        setCommand(Motion::Forward);
    } else if (clamped < 0.0) {
        setCommand(Motion::Backward);
    } else {
        stop();
    }
}

void MotorController::setDynamics(const DynamicsConfig &config) {
    invert_left_ = config.invert_left;
    invert_right_ = config.invert_right;
    command_timeout_ms_.store(std::max(config.command_timeout_ms, 0));
}

void MotorController::initializePins() {
    pinMode(forward_pin_left_, OUTPUT);
    pinMode(backward_pin_left_, OUTPUT);
    pinMode(forward_pin_right_, OUTPUT);
    pinMode(backward_pin_right_, OUTPUT);

    digitalWrite(forward_pin_left_, LOW);
    digitalWrite(backward_pin_left_, LOW);
    digitalWrite(forward_pin_right_, LOW);
    digitalWrite(backward_pin_right_, LOW);
}

void MotorController::controlLoop() {
    while (running_.load()) {
        Motion desired = Motion::Stopped;
        bool should_apply = false;
        auto now = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(command_timeout_ms_.load());

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (command_active_) {
                if (timeout.count() == 0 || now - last_command_time_ <= timeout) {
                    desired = requested_motion_;
                } else {
                    command_active_ = false;
                    requested_motion_ = Motion::Stopped;
                    desired = Motion::Stopped;
                }
            } else {
                desired = requested_motion_;
            }

            if (desired != applied_motion_) {
                applied_motion_ = desired;
                should_apply = true;
            }
        }

        if (should_apply) {
            applyMotion(desired);
        }

        std::this_thread::sleep_for(kControlInterval);
    }

    applyMotion(Motion::Stopped);
}

void MotorController::setCommand(Motion motion) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(state_mutex_);
    requested_motion_ = motion;
    command_active_ = true;
    last_command_time_ = now;
}

void MotorController::applyMotion(Motion motion) {
    applyMotor(forward_pin_left_, backward_pin_left_, motion, invert_left_);
    applyMotor(forward_pin_right_, backward_pin_right_, motion, invert_right_);
}

void MotorController::applyMotor(int forward_pin, int backward_pin, Motion motion, bool invert) {
    Motion effective_motion = motion;
    if (invert) {
        if (motion == Motion::Forward) {
            effective_motion = Motion::Backward;
        } else if (motion == Motion::Backward) {
            effective_motion = Motion::Forward;
        }
    }

    bool forward = effective_motion == Motion::Forward;
    bool backward = effective_motion == Motion::Backward;

    digitalWrite(forward_pin, forward ? HIGH : LOW);
    digitalWrite(backward_pin, backward ? HIGH : LOW);
}

} // namespace autonomous_car::controllers
