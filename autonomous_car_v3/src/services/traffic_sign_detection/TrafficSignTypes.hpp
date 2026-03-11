#pragma once

#include <cstdint>
#include <optional>
#include <opencv2/core.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace autonomous_car::services::traffic_sign_detection {

enum class TrafficSignId { Stop, TurnLeft, TurnRight };

enum class TrafficSignDetectorState { Disabled, Idle, Candidate, Confirmed, Error };

struct DetectionBox {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct TrafficSignRoi {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    double right_width_ratio{0.35};
    double top_ratio{0.0};
    double bottom_ratio{1.0};
};

struct TrafficSignDetection {
    TrafficSignId sign_id{TrafficSignId::Stop};
    std::string model_label;
    std::string display_label;
    double confidence_score{0.0};
    DetectionBox bbox_frame;
    DetectionBox bbox_roi;
};

struct StabilizedTrafficSignDetection : TrafficSignDetection {
    int consecutive_frames{0};
    int required_frames{0};
    std::int64_t confirmed_at_ms{0};
    std::int64_t last_seen_at_ms{0};
};

struct TrafficSignDetectionFrame {
    bool enabled{true};
    TrafficSignRoi roi;
    std::vector<TrafficSignDetection> raw_detections;
    std::string last_error;
};

struct TrafficSignFrameResult {
    bool enabled{true};
    TrafficSignDetectorState detector_state{TrafficSignDetectorState::Idle};
    TrafficSignRoi roi;
    std::vector<TrafficSignDetection> raw_detections;
    std::optional<StabilizedTrafficSignDetection> candidate;
    std::optional<StabilizedTrafficSignDetection> active_detection;
    std::string last_error;
};

std::string_view toString(TrafficSignId id);
std::string_view toString(TrafficSignDetectorState state);
std::string displayLabel(TrafficSignId id);
std::optional<TrafficSignId> trafficSignIdFromModelLabel(std::string_view label);
std::string buildModelCompatibilityWarning(const std::vector<std::string> &model_categories);
cv::Rect toCvRect(const DetectionBox &box);
double detectionArea(const TrafficSignDetection &detection);
bool isBetterDetection(const TrafficSignDetection &lhs, const TrafficSignDetection &rhs);

} // namespace autonomous_car::services::traffic_sign_detection
