#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignDetector.hpp"

namespace autonomous_car::services::traffic_sign_detection {

class EdgeImpulseTrafficSignDetector : public TrafficSignDetector {
public:
    explicit EdgeImpulseTrafficSignDetector(TrafficSignConfig config);
    ~EdgeImpulseTrafficSignDetector() override = default;

    TrafficSignFrameResult detect(const cv::Mat &frame,
                                  std::int64_t timestamp_ms) override;
    TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                  std::int64_t timestamp_ms) override;

private:
    bool validateModelLabels(std::string &error_message) const;
    TrafficSignFrameResult makeErrorResult(const cv::Size &frame_size,
                                           std::int64_t timestamp_ms,
                                           const std::string &error_message) const;
    void prepareInputBuffer(const cv::Mat &roi_frame);

    TrafficSignConfig config_;
    bool model_ready_{false};
    std::string last_error_;
    cv::Mat grayscale_buffer_;
    cv::Mat resized_buffer_;
    std::vector<float> input_buffer_;
};

} // namespace autonomous_car::services::traffic_sign_detection
