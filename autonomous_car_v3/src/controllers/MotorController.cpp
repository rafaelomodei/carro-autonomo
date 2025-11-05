#include "controllers/MotorController.hpp"

#include <algorithm>
#include <cmath>

#include <wiringPi.h>

namespace autonomous_car::controllers {

namespace {
constexpr double kMinNormalizedThrottle = -1.0;
constexpr double kMaxNormalizedThrottle = 1.0;
}

MotorController::MotorController(int forward_pin_left, int backward_pin_left,
                                 int forward_pin_right, int backward_pin_right)
    : forward_pin_left_{forward_pin_left},
      backward_pin_left_{backward_pin_left},
      forward_pin_right_{forward_pin_right},
      backward_pin_right_{backward_pin_right},
      invert_left_{false},
      invert_right_{true},
      running_{true},
      control_interval_ms_{20},
      max_delta_per_interval_{0.15},
      min_active_throttle_{0.2},
      target_throttle_{0.0},
      current_throttle_{0.0} {
    initializePins();

    pid_.setCoefficients(2.5, 0.0, 0.35);
    pid_.setOutputLimits(-max_delta_per_interval_.load(), max_delta_per_interval_.load());
    pid_.reset();

    control_thread_ = std::thread(&MotorController::controlLoop, this);
}

MotorController::~MotorController() {
    running_.store(false);
    if (control_thread_.joinable()) {
        control_thread_.join();
    }

    applyThrottle(0.0);
    digitalWrite(forward_pin_left_, LOW);
    digitalWrite(backward_pin_left_, LOW);
    digitalWrite(forward_pin_right_, LOW);
    digitalWrite(backward_pin_right_, LOW);
}

void MotorController::forward(double intensity) {
    double normalized = std::clamp(intensity, 0.0, 1.0);
    setThrottle(normalized);
}

void MotorController::backward(double intensity) {
    double normalized = -std::clamp(std::abs(intensity), 0.0, 1.0);
    setThrottle(normalized);
}

void MotorController::stop() {
    setThrottle(0.0);
}

void MotorController::setThrottle(double normalized_value) {
    double clamped = std::clamp(normalized_value, kMinNormalizedThrottle, kMaxNormalizedThrottle);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        target_throttle_ = clamped;
    }
}

void MotorController::setDynamics(const DynamicsConfig &config) {
    invert_left_ = config.invert_left;
    invert_right_ = config.invert_right;
    control_interval_ms_.store(std::max(config.control_interval_ms, 5));
    double limit = std::clamp(std::abs(config.output_limit), 0.01, 1.0);
    max_delta_per_interval_.store(limit);
    double min_active = std::clamp(std::abs(config.min_active_throttle), 0.0, 1.0);
    min_active_throttle_.store(min_active);
    pid_.setCoefficients(config.kp, config.ki, config.kd);
    pid_.setOutputLimits(-limit, limit);
    pid_.reset();
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
    auto previous = std::chrono::steady_clock::now();
    while (running_.load()) {
        auto interval = std::chrono::milliseconds(control_interval_ms_.load());
        std::this_thread::sleep_for(interval);

        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - previous).count();
        previous = now;

        double target = 0.0;
        double current = 0.0;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            target = target_throttle_;
            current = current_throttle_;
        }

        double desired_change = target - current;
        double max_delta = max_delta_per_interval_.load();
        double pid_delta = pid_.compute(target, current, dt);
        double clamped_delta = std::clamp(pid_delta, -max_delta, max_delta);
        double fallback_delta = std::clamp(desired_change, -max_delta, max_delta);

        if (desired_change > 0.0) {
            if (clamped_delta <= 0.0) {
                clamped_delta = std::max(0.0, fallback_delta);
            }
        } else if (desired_change < 0.0) {
            if (clamped_delta >= 0.0) {
                clamped_delta = std::min(0.0, fallback_delta);
            }
        }

        double updated = std::clamp(current + clamped_delta, kMinNormalizedThrottle, kMaxNormalizedThrottle);
        applyThrottle(updated);

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            current_throttle_ = updated;
        }
    }

    applyThrottle(0.0);
}

void MotorController::applyThrottle(double throttle) {
    applyMotor(forward_pin_left_, backward_pin_left_, throttle, invert_left_);
    applyMotor(forward_pin_right_, backward_pin_right_, throttle, invert_right_);
}

void MotorController::applyMotor(int forward_pin, int backward_pin, double throttle, bool invert) {
    double effective = invert ? -throttle : throttle;
    double threshold = std::max(min_active_throttle_.load(), 0.0);

    bool forward = false;
    bool backward = false;
    if (threshold <= 0.0) {
        forward = effective > 0.0;
        backward = effective < 0.0;
    } else {
        forward = effective >= threshold;
        backward = effective <= -threshold;
    }

    if (!forward && !backward) {
        digitalWrite(forward_pin, LOW);
        digitalWrite(backward_pin, LOW);
        return;
    }

    digitalWrite(forward_pin, forward ? HIGH : LOW);
    digitalWrite(backward_pin, backward ? HIGH : LOW);
}

} // namespace autonomous_car::controllers
