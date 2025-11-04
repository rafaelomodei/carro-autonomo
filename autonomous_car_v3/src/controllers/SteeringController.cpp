#include "controllers/SteeringController.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <softPwm.h>

namespace autonomous_car::controllers {

namespace {
constexpr int kDefaultMinPwm = 5;   // ~0.5ms pulse width
constexpr int kDefaultMaxPwm = 25;  // ~2.5ms pulse width
constexpr int kCenterAngle = 90;
constexpr int kBaseSteeringDelta = 45;
constexpr double kMinNormalizedSteering = -1.0;
constexpr double kMaxNormalizedSteering = 1.0;
}

SteeringController::SteeringController(int pwm_pin, int min_angle, int max_angle)
    : pwm_pin_{pwm_pin},
      min_angle_{min_angle},
      max_angle_{max_angle},
      center_angle_{kCenterAngle},
      min_pwm_{kDefaultMinPwm},
      max_pwm_{kDefaultMaxPwm},
      steering_sensitivity_{1.0},
      running_{true},
      control_interval_ms_{20},
      target_offset_{0.0},
      current_offset_{0.0} {
    initializePwm();
    pid_.setOutputLimits(-0.2, 0.2);
    pid_.reset();

    control_thread_ = std::thread(&SteeringController::controlLoop, this);
}

SteeringController::~SteeringController() {
    running_.store(false);
    if (control_thread_.joinable()) {
        control_thread_.join();
    }

    applyAngle(center_angle_);
    softPwmStop(pwm_pin_);
}

void SteeringController::turnLeft(double intensity) {
    double normalized = -std::clamp(std::abs(intensity), 0.0, 1.0);
    setSteering(normalized);
}

void SteeringController::turnRight(double intensity) {
    double normalized = std::clamp(intensity, 0.0, 1.0);
    setSteering(normalized);
}

void SteeringController::center() {
    setSteering(0.0);
}

void SteeringController::setSteering(double normalized_value) {
    double clamped = std::clamp(normalized_value, kMinNormalizedSteering, kMaxNormalizedSteering);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        target_offset_ = clamped;
    }
}

void SteeringController::setAngle(int angle) {
    int clamped_angle = clampAngle(angle);
    int delta = steeringDelta();
    if (delta <= 0) {
        setSteering(0.0);
        return;
    }
    double normalized = static_cast<double>(clamped_angle - center_angle_) /
                        static_cast<double>(delta);
    normalized = std::clamp(normalized, kMinNormalizedSteering, kMaxNormalizedSteering);
    setSteering(normalized);
}

void SteeringController::setSteeringSensitivity(double sensitivity) {
    if (!std::isfinite(sensitivity) || sensitivity <= 0.0) {
        return;
    }
    steering_sensitivity_.store(sensitivity);
}

void SteeringController::setDynamics(const DynamicsConfig &config) {
    pid_.setCoefficients(config.kp, config.ki, config.kd);
    double limit = std::clamp(std::abs(config.output_limit), 0.01, 1.0);
    pid_.setOutputLimits(-limit, limit);
    control_interval_ms_.store(std::max(config.control_interval_ms, 5));
    pid_.reset();
}

void SteeringController::initializePwm() {
    if (softPwmCreate(pwm_pin_, 0, 200) != 0) {
        std::cerr << "Falha ao iniciar PWM por software no pino " << pwm_pin_ << std::endl;
    }
    applyAngle(center_angle_);
}

void SteeringController::controlLoop() {
    auto previous = std::chrono::steady_clock::now();
    while (running_.load()) {
        auto interval = std::chrono::milliseconds(control_interval_ms_.load());
        std::this_thread::sleep_for(interval);

        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - previous).count();
        previous = now;

        double target = 0.0;
        double current = 0.0;
        double sensitivity = steering_sensitivity_.load();
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            target = target_offset_;
            current = current_offset_;
        }

        double delta = pid_.compute(target, current, dt);
        double updated = std::clamp(current + delta, kMinNormalizedSteering, kMaxNormalizedSteering);

        int delta_angle = static_cast<int>(std::round(kBaseSteeringDelta * sensitivity));
        delta_angle = std::max(delta_angle, 1);
        int angle = center_angle_ + static_cast<int>(std::round(updated * delta_angle));
        angle = clampAngle(angle);
        applyAngle(angle);

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            current_offset_ = updated;
        }
    }

    applyAngle(center_angle_);
}

void SteeringController::applyAngle(int angle) {
    int clamped = clampAngle(angle);
    int pwm_value = toPwmValue(clamped);
    softPwmWrite(pwm_pin_, pwm_value);
}

int SteeringController::clampAngle(int angle) const {
    return std::clamp(angle, min_angle_, max_angle_);
}

int SteeringController::toPwmValue(int angle) const {
    if (max_angle_ == min_angle_) {
        return min_pwm_;
    }
    double proportion = static_cast<double>(angle - min_angle_) /
                         static_cast<double>(max_angle_ - min_angle_);
    int range = max_pwm_ - min_pwm_;
    return min_pwm_ + static_cast<int>(proportion * range);
}

int SteeringController::steeringDelta() const {
    double sensitivity = steering_sensitivity_.load();
    int delta = static_cast<int>(std::round(kBaseSteeringDelta * sensitivity));
    return std::max(delta, 1);
}

} // namespace autonomous_car::controllers
