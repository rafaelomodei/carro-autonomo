#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "config/ServiceConfig.hpp"
#include "domain/ITrafficSignClassifier.hpp"
#include "edge_impulse/ImageSignalEncoder.hpp"
#include <sys/types.h>

namespace traffic_sign_service::edge_impulse {

class EimProcessClassifier final : public ITrafficSignClassifier {
public:
    explicit EimProcessClassifier(config::ServiceConfig config);
    ~EimProcessClassifier() override;

    TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                  std::int64_t timestamp_ms) override;
    std::vector<std::string> modelLabels() const override;
    cv::Size inputSize() const override;
    std::string backendName() const override;

private:
    struct ModelInfo {
        int input_width{0};
        int input_height{0};
        int image_channel_count{0};
        int input_features_count{0};
        std::string model_type;
        ImageResizeMode resize_mode{ImageResizeMode::Squash};
        std::vector<std::string> labels;
    };

    void ensureProcessStarted();
    void stopProcess();
    void sendJson(const std::string &payload);
    std::optional<std::string> readLine();
    std::optional<nlohmann::json> readResponse(int expected_id);
    ModelInfo loadModelInfo() const;
    TrafficSignFrameResult makeErrorResult(const cv::Size &frame_size, std::int64_t timestamp_ms,
                                           const std::string &error_message) const;
    void attachDebugInfo(TrafficSignFrameResult &frame_result, const cv::Mat &frame,
                         const cv::Mat &roi_frame, const cv::Mat &model_input,
                         bool capture_debug_frames) const;
    EncodedImageSignal prepareModelInput(const cv::Mat &roi_frame) const;

    config::ServiceConfig config_;
    ModelInfo model_info_;
    std::string model_labels_summary_;
    std::string model_input_summary_;
    std::string startup_error_;
    int next_request_id_{1};
    pid_t child_pid_{-1};
    FILE *child_stdin_{nullptr};
    FILE *child_stdout_{nullptr};
};

} // namespace traffic_sign_service::edge_impulse
