#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <opencv2/core.hpp>

namespace autonomous_car::services::traffic_sign_detection {

enum class TrafficSignId {
    Stop,
    TurnLeft,
    TurnRight,
    Unknown,
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

struct TrafficSignDetection {
    TrafficSignId sign_id{TrafficSignId::Unknown};
    std::string model_label;
    std::string display_label;
    double confidence_score{0.0};
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
    std::vector<TrafficSignDetection> raw_detections;
    std::optional<TrafficSignDetection> candidate;
    std::optional<TrafficSignDetection> active_detection;
    std::string model_labels_summary;
    cv::Mat debug_roi_frame;
    cv::Mat debug_model_input_frame;
    std::string last_error;
};

std::string_view toString(TrafficSignId sign_id);
std::string_view toString(TrafficSignDetectorState state);
std::string displayLabel(TrafficSignId sign_id);
std::string normalizeModelLabel(std::string_view label);
TrafficSignId trafficSignIdFromModelLabel(std::string_view label);
double trafficSignRoiRightWidthRatio(const TrafficSignRoi &roi);
TrafficSignRoi buildTrafficSignRoi(cv::Size frame_size, double left_ratio, double right_ratio,
                                   double top_ratio, double bottom_ratio,
                                   bool debug_roi_enabled = true);
TrafficSignRoi buildTrafficSignRoi(cv::Size frame_size, double right_width_ratio,
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

} // namespace autonomous_car::services::traffic_sign_detection
