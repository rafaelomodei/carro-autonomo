#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <opencv2/core.hpp>

#include "domain/TrafficSignal.hpp"

namespace traffic_sign_service {

enum class FrameSourceMode {
    Image,
    Video,
    Camera,
    WebSocket,
};

enum class EdgeImpulseBackend {
    Eim,
    EmbeddedCpp,
};

enum class TrafficSignDetectorState {
    Disabled,
    Idle,
    Candidate,
    Confirmed,
    Error,
};

struct TrafficSignBoundingBox {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct TrafficSignRoi {
    double left_ratio{0.55};
    double right_ratio{1.0};
    double top_ratio{0.08};
    double bottom_ratio{0.72};
    bool debug_roi_enabled{true};
    cv::Size source_frame_size;
    cv::Rect frame_rect;
};

struct SignalDetection {
    TrafficSignalId signal_id{TrafficSignalId::Unknown};
    std::string model_label;
    std::string display_label;
    double confidence{0.0};
    TrafficSignBoundingBox bbox_frame;
    TrafficSignBoundingBox bbox_roi;
    int consecutive_frames{0};
    int required_frames{0};
    std::int64_t confirmed_at_ms{0};
    std::int64_t last_seen_at_ms{0};
};

struct TrafficSignFrameResult {
    std::int64_t timestamp_ms{0};
    TrafficSignDetectorState detector_state{TrafficSignDetectorState::Idle};
    TrafficSignRoi roi;
    std::vector<SignalDetection> raw_detections;
    std::optional<SignalDetection> candidate;
    std::optional<SignalDetection> active_detection;
    std::vector<std::string> model_labels;
    std::string model_labels_summary;
    std::string model_input_summary;
    cv::Mat debug_frame;
    cv::Mat debug_roi_frame;
    cv::Mat debug_model_input_frame;
    std::string last_error;
};

struct TrafficSignInferenceInput {
    cv::Mat frame;
    cv::Size full_frame_size;
    std::optional<TrafficSignRoi> roi;
    bool capture_debug_frames{false};
};

struct TrafficSignRuntimeMetrics {
    std::string source_label;
    std::string source_mode;
    std::string backend;
    double service_fps{0.0};
    double inference_ms{0.0};
    std::int64_t sign_result_age_ms{-1};
    std::uint64_t frames_received{0};
    std::uint64_t frames_processed{0};
    std::uint64_t dropped_frames{0};
};

std::string_view toString(FrameSourceMode mode);
std::optional<FrameSourceMode> frameSourceModeFromString(std::string_view value);

std::string_view toString(EdgeImpulseBackend backend);
std::optional<EdgeImpulseBackend> edgeImpulseBackendFromString(std::string_view value);

std::string_view toString(TrafficSignDetectorState state);

double trafficSignRoiRightWidthRatio(const TrafficSignRoi &roi);
TrafficSignRoi buildTrafficSignRoi(cv::Size frame_size, double left_ratio, double right_ratio,
                                   double top_ratio, double bottom_ratio,
                                   bool debug_roi_enabled = true);
TrafficSignBoundingBox scaleBoundingBox(const TrafficSignBoundingBox &bbox, cv::Size from_size,
                                        cv::Size to_size);
TrafficSignBoundingBox translateBoundingBox(const TrafficSignBoundingBox &bbox,
                                            const cv::Point &offset);
TrafficSignBoundingBox clampBoundingBox(const TrafficSignBoundingBox &bbox,
                                        const cv::Rect &bounds);
cv::Rect toCvRect(const TrafficSignBoundingBox &bbox);
TrafficSignFrameResult makeTrafficSignFrameResult(TrafficSignDetectorState state,
                                                  const TrafficSignRoi &roi,
                                                  std::int64_t timestamp_ms,
                                                  std::string last_error = {});
std::int64_t trafficSignResultAgeMs(const TrafficSignFrameResult &result,
                                    std::int64_t reference_timestamp_ms);

} // namespace traffic_sign_service
