#pragma once

#include <atomic>
#include <mutex>

namespace autonomous_car::controllers {

class SteeringController {
public:
    struct AngleLimitConfig {
        int center_angle{90};
        int left_range{20};
        int right_range{20};
    };

    SteeringController(int pwm_pin, int servo_min_angle = 0, int servo_max_angle = 180);
    ~SteeringController();

    void turnLeft(double intensity = 1.0);
    void turnRight(double intensity = 1.0);
    void center();
    void setSteering(double normalized_value);
    void setAngle(int angle);

    void setSteeringSensitivity(double sensitivity);
    void setCommandStep(double step);
    void configureAngleLimits(const AngleLimitConfig &config);
    void configureAngleLimits(int center_angle, int left_range, int right_range);

    struct AngleLimitState {
        int center_angle{90};
        int left_range{20};
        int right_range{20};
        int min_angle{70};
        int max_angle{110};
    };

private:
    void initializePwm();
    void applyCurrentSteering();
    void applySmoothedSteering(double target, double sensitivity, const AngleLimitState &limits);
    double applySensitivity(double normalized_value, double sensitivity) const;
    AngleLimitState computeAngleState(const AngleLimitConfig &config) const;
    AngleLimitState computeAngleState(int center_angle, int left_range, int right_range) const;
    AngleLimitState loadAngleLimits() const;
    int normalizedToAngle(double normalized, const AngleLimitState &limits) const;
    double angleToNormalized(int angle, const AngleLimitState &limits) const;
    void applyAngle(int angle, const AngleLimitState &limits);
    int toPwmValue(int angle) const;
    void nudgeTarget(double delta);

    int pwm_pin_;
    int servo_min_angle_;
    int servo_max_angle_;
    int min_pwm_;
    int max_pwm_;
    std::atomic<double> steering_sensitivity_;
    std::atomic<double> command_step_;

    mutable std::mutex state_mutex_;
    AngleLimitState angle_limits_;
    double target_offset_;
};

} // namespace autonomous_car::controllers
