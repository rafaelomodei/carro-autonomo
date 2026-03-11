#pragma once

#include <opencv2/core.hpp>

#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class EdgeImpulseTrafficSignDetector {
public:
    explicit EdgeImpulseTrafficSignDetector(TrafficSignConfig config = {});

    void updateConfig(const TrafficSignConfig &config);
    [[nodiscard]] const TrafficSignConfig &config() const noexcept;
    [[nodiscard]] TrafficSignDetectionFrame detect(const cv::Mat &frame) const;

private:
    TrafficSignConfig config_;
    std::string model_warning_;
};

} // namespace autonomous_car::services::traffic_sign_detection
