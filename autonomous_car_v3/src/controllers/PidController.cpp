#include "controllers/PidController.hpp"

#include <cmath>

namespace autonomous_car::controllers {

namespace {
constexpr double kDefaultKp = 0.0;
constexpr double kDefaultKi = 0.0;
constexpr double kDefaultKd = 0.0;
constexpr double kDefaultMinOutput = -1.0;
constexpr double kDefaultMaxOutput = 1.0;
constexpr double kDefaultMinIntegral = -1.0;
constexpr double kDefaultMaxIntegral = 1.0;
}

PidController::PidController()
    : kp_{kDefaultKp},
      ki_{kDefaultKi},
      kd_{kDefaultKd},
      min_output_{kDefaultMinOutput},
      max_output_{kDefaultMaxOutput},
      min_integral_{kDefaultMinIntegral},
      max_integral_{kDefaultMaxIntegral},
      integral_{0.0},
      previous_error_{0.0},
      has_previous_error_{false} {}

void PidController::setCoefficients(double kp, double ki, double kd) {
    kp_ = std::isfinite(kp) ? kp : kp_;
    ki_ = std::isfinite(ki) ? ki : ki_;
    kd_ = std::isfinite(kd) ? kd : kd_;
}

void PidController::setOutputLimits(double min_output, double max_output) {
    if (!std::isfinite(min_output) || !std::isfinite(max_output)) {
        return;
    }
    if (min_output > max_output) {
        std::swap(min_output, max_output);
    }
    min_output_ = min_output;
    max_output_ = max_output;
}

void PidController::setIntegralLimits(double min_integral, double max_integral) {
    if (!std::isfinite(min_integral) || !std::isfinite(max_integral)) {
        return;
    }
    if (min_integral > max_integral) {
        std::swap(min_integral, max_integral);
    }
    min_integral_ = min_integral;
    max_integral_ = max_integral;
}

void PidController::reset() {
    integral_ = 0.0;
    previous_error_ = 0.0;
    has_previous_error_ = false;
}

double PidController::compute(double target, double measurement, double dt_seconds) {
    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0) {
        return 0.0;
    }

    double error = target - measurement;
    integral_ += error * dt_seconds;
    integral_ = std::clamp(integral_, min_integral_, max_integral_);

    double derivative = 0.0;
    if (has_previous_error_) {
        derivative = (error - previous_error_) / dt_seconds;
    }

    previous_error_ = error;
    has_previous_error_ = true;

    double output = kp_ * error + ki_ * integral_ + kd_ * derivative;
    return std::clamp(output, min_output_, max_output_);
}

} // namespace autonomous_car::controllers
