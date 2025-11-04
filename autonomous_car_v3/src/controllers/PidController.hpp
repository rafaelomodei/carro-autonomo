#pragma once

#include <algorithm>

namespace autonomous_car::controllers {

class PidController {
public:
    PidController();

    void setCoefficients(double kp, double ki, double kd);
    void setOutputLimits(double min_output, double max_output);
    void setIntegralLimits(double min_integral, double max_integral);

    void reset();
    double compute(double target, double measurement, double dt_seconds);

private:
    double kp_;
    double ki_;
    double kd_;

    double min_output_;
    double max_output_;

    double min_integral_;
    double max_integral_;

    double integral_;
    double previous_error_;
    bool has_previous_error_;
};

} // namespace autonomous_car::controllers
