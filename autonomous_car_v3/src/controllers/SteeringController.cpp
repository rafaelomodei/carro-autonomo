#include "controllers/SteeringController.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <softPwm.h>

namespace autonomous_car::controllers {

namespace {
constexpr int kDefaultMinPwm = 5;   // ~0.5ms pulse width
constexpr int kDefaultMaxPwm = 25;  // ~2.5ms pulse width
constexpr double kMinNormalizedSteering = -1.0;
constexpr double kMaxNormalizedSteering = 1.0;
}

SteeringController::SteeringController(int pwm_pin, int servo_min_angle, int servo_max_angle)
    : pwm_pin_{pwm_pin},
      servo_min_angle_{servo_min_angle},
      servo_max_angle_{servo_max_angle},
      min_pwm_{kDefaultMinPwm},
      max_pwm_{kDefaultMaxPwm},
      steering_sensitivity_{1.0},
      command_step_{0.1},
      running_{true},
      control_interval_ms_{80},
      target_offset_{0.0},
      current_offset_{0.0} {
    angle_limits_ = computeAngleState(AngleLimitConfig{});
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

    auto limits = loadAngleLimits();
    applyAngle(limits.center_angle, limits);
    softPwmStop(pwm_pin_);
}

void SteeringController::turnLeft(double intensity) {
    double sanitized = std::abs(intensity);
    if (sanitized <= 0.0) {
        sanitized = 1.0;
    }
    sanitized = std::clamp(sanitized, 0.0, 1.0);
    double step = command_step_.load();
    if (step <= 0.0) {
        step = 0.1;
    }
    nudgeTarget(-step * sanitized);
}

void SteeringController::turnRight(double intensity) {
    double sanitized = std::abs(intensity);
    if (sanitized <= 0.0) {
        sanitized = 1.0;
    }
    sanitized = std::clamp(sanitized, 0.0, 1.0);
    double step = command_step_.load();
    if (step <= 0.0) {
        step = 0.1;
    }
    nudgeTarget(step * sanitized);
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
    auto limits = loadAngleLimits();
    int clamped_angle = std::clamp(angle, limits.min_angle, limits.max_angle);
    double normalized = angleToNormalized(clamped_angle, limits);
    setSteering(normalized);
}

void SteeringController::setSteeringSensitivity(double sensitivity) {
    if (!std::isfinite(sensitivity) || sensitivity <= 0.0) {
        return;
    }
    steering_sensitivity_.store(sensitivity);
}

void SteeringController::setCommandStep(double step) {
    if (!std::isfinite(step) || step <= 0.0) {
        return;
    }
    command_step_.store(std::min(step, 1.0));
}

void SteeringController::setDynamics(const DynamicsConfig &config) {
    pid_.setCoefficients(config.kp, config.ki, config.kd);
    double limit = std::clamp(std::abs(config.output_limit), 0.01, 1.0);
    pid_.setOutputLimits(-limit, limit);
    control_interval_ms_.store(std::max(config.control_interval_ms, 5));
    pid_.reset();
}

void SteeringController::configureAngleLimits(const AngleLimitConfig &config) {
    configureAngleLimits(config.center_angle, config.left_range, config.right_range);
}

void SteeringController::configureAngleLimits(int center_angle, int left_range, int right_range) {
    AngleLimitState updated = computeAngleState(center_angle, left_range, right_range);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        angle_limits_ = updated;
        target_offset_ = 0.0;
        current_offset_ = 0.0;
    }
    applyAngle(updated.center_angle, updated);
}

void SteeringController::initializePwm() {
    if (softPwmCreate(pwm_pin_, 0, 200) != 0) {
        std::cerr << "Falha ao iniciar PWM por software no pino " << pwm_pin_ << std::endl;
    }
    auto limits = loadAngleLimits();
    applyAngle(limits.center_angle, limits);
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
        AngleLimitState limits;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            target = target_offset_;
            current = current_offset_;
            limits = angle_limits_;
        }

        double delta = pid_.compute(target, current, dt);
        double updated = std::clamp(current + delta, kMinNormalizedSteering, kMaxNormalizedSteering);
        double scaled = std::clamp(updated * sensitivity, kMinNormalizedSteering, kMaxNormalizedSteering);

        int angle = normalizedToAngle(scaled, limits);
        applyAngle(angle, limits);

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            current_offset_ = updated;
        }
    }

    auto limits = loadAngleLimits();
    applyAngle(limits.center_angle, limits);
}

SteeringController::AngleLimitState SteeringController::computeAngleState(const AngleLimitConfig &config) const {
    return computeAngleState(config.center_angle, config.left_range, config.right_range);
}

SteeringController::AngleLimitState SteeringController::computeAngleState(int center_angle, int left_range,
                                                                         int right_range) const {
    AngleLimitState state;
    int clamped_center = std::clamp(center_angle, servo_min_angle_, servo_max_angle_);
    int max_left = clamped_center - servo_min_angle_;
    int max_right = servo_max_angle_ - clamped_center;

    state.center_angle = clamped_center;
    state.left_range = std::clamp(left_range, 0, max_left);
    state.right_range = std::clamp(right_range, 0, max_right);
    state.min_angle = state.center_angle - state.left_range;
    state.max_angle = state.center_angle + state.right_range;
    return state;
}

SteeringController::AngleLimitState SteeringController::loadAngleLimits() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return angle_limits_;
}

int SteeringController::normalizedToAngle(double normalized, const AngleLimitState &limits) const {
    double clamped = std::clamp(normalized, kMinNormalizedSteering, kMaxNormalizedSteering);
    if (clamped >= 0.0) {
        int range = std::max(limits.right_range, 0);
        return limits.center_angle + static_cast<int>(std::round(clamped * range));
    }
    int range = std::max(limits.left_range, 0);
    return limits.center_angle + static_cast<int>(std::round(clamped * range));
}

double SteeringController::angleToNormalized(int angle, const AngleLimitState &limits) const {
    int clamped_angle = std::clamp(angle, limits.min_angle, limits.max_angle);
    if (clamped_angle >= limits.center_angle) {
        int denominator = std::max(limits.right_range, 1);
        double delta = static_cast<double>(clamped_angle - limits.center_angle) / denominator;
        return std::clamp(delta, 0.0, kMaxNormalizedSteering);
    }
    int denominator = std::max(limits.left_range, 1);
    double delta = static_cast<double>(clamped_angle - limits.center_angle) / denominator;
    return std::clamp(delta, kMinNormalizedSteering, 0.0);
}

void SteeringController::applyAngle(int angle, const AngleLimitState &limits) {
    int clamped = std::clamp(angle, limits.min_angle, limits.max_angle);
    int pwm_value = toPwmValue(clamped);
    softPwmWrite(pwm_pin_, pwm_value);
}

void SteeringController::nudgeTarget(double delta) {
    if (!std::isfinite(delta) || delta == 0.0) {
        return;
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    double next = std::clamp(target_offset_ + delta, kMinNormalizedSteering, kMaxNormalizedSteering);
    target_offset_ = next;
}

int SteeringController::toPwmValue(int angle) const {
    if (servo_max_angle_ == servo_min_angle_) {
        return min_pwm_;
    }
    double proportion = static_cast<double>(angle - servo_min_angle_) /
                         static_cast<double>(servo_max_angle_ - servo_min_angle_);
    int range = max_pwm_ - min_pwm_;
    return min_pwm_ + static_cast<int>(proportion * range);
}

} // namespace autonomous_car::controllers
