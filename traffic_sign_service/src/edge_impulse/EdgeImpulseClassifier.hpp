#pragma once

#include "domain/ITrafficSignClassifier.hpp"
#include "config/ServiceConfig.hpp"
#include "edge_impulse/ImageSignalEncoder.hpp"

namespace traffic_sign_service::edge_impulse {

class EdgeImpulseClassifier : public ITrafficSignClassifier {
public:
    explicit EdgeImpulseClassifier(config::ServiceConfig config);

    TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                  std::int64_t timestamp_ms) override;
    std::vector<std::string> modelLabels() const override;
    cv::Size inputSize() const override;
    std::string backendName() const override;

private:
    bool validateModelLabels(std::string &error_message) const;
    TrafficSignFrameResult makeErrorResult(const cv::Size &frame_size, std::int64_t timestamp_ms,
                                           const std::string &error_message) const;
    void attachModelDebugInfo(TrafficSignFrameResult &frame_result, const cv::Mat &frame,
                              const cv::Mat &roi_frame, bool capture_debug_frames);
    void prepareInputBuffer(const cv::Mat &roi_frame);

    config::ServiceConfig config_;
    bool model_ready_{false};
    std::string last_error_;
    std::vector<std::string> model_labels_;
    std::string model_labels_summary_;
    std::string model_input_summary_;
    std::vector<float> input_buffer_;
    cv::Mat resized_buffer_;
};

} // namespace traffic_sign_service::edge_impulse
