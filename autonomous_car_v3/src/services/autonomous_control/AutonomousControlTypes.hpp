#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "common/DrivingMode.hpp"

namespace autonomous_car::services::autonomous_control {

enum class TrackingState { Manual, Idle, Searching, Tracking, FailSafe };
enum class StopReason { None, CommandStop, ModeChange, LaneLost, LowConfidence, ServiceStop };
enum class MotionCommand { Stopped, Forward };

struct ReferenceControlState {
    bool valid{false};
    bool used{false};
    double configured_weight{0.0};
    double applied_weight{0.0};
    double steering_error_normalized{0.0};
    double lateral_offset_px{0.0};
};

struct TrajectoryPoint {
    double x{0.0};
    double y{0.0};
};

struct AutonomousControlConfig {
    double pid_kp{0.45};
    double pid_ki{0.02};
    double pid_kd{0.08};
    double pid_output_limit{0.55};
    double preview_near_weight{0.5};
    double preview_mid_weight{0.3};
    double preview_far_weight{0.2};
    double max_steering_delta_per_update{0.08};
    double min_confidence{0.25};
    int lane_loss_timeout_ms{250};
};

struct AutonomousPidState {
    double error{0.0};
    double proportional{0.0};
    double integral{0.0};
    double derivative{0.0};
    double raw_output{0.0};
    double pid_output{0.0};
};

struct AutonomousControlSnapshot {
    DrivingMode driving_mode{DrivingMode::Manual};
    bool autonomous_started{false};
    bool fail_safe_active{false};
    TrackingState tracking_state{TrackingState::Manual};
    StopReason stop_reason{StopReason::None};
    MotionCommand motion_command{MotionCommand::Stopped};
    bool lane_available{false};
    bool confidence_ok{false};
    double confidence_score{0.0};
    ReferenceControlState near_reference;
    ReferenceControlState mid_reference;
    ReferenceControlState far_reference;
    AutonomousPidState pid;
    double preview_error{0.0};
    double steering_command{0.0};
    double heading_error_rad{0.0};
    bool heading_valid{false};
    double curvature_indicator_rad{0.0};
    bool curvature_valid{false};
    std::int64_t timestamp_ms{0};
    std::int64_t last_tracking_timestamp_ms{0};
    std::vector<TrajectoryPoint> projected_path;
};

inline std::string_view toString(TrackingState state) {
    switch (state) {
    case TrackingState::Manual:
        return "manual";
    case TrackingState::Idle:
        return "idle";
    case TrackingState::Searching:
        return "searching";
    case TrackingState::Tracking:
        return "tracking";
    case TrackingState::FailSafe:
        return "fail_safe";
    }
    return "unknown";
}

inline std::string_view toString(StopReason reason) {
    switch (reason) {
    case StopReason::None:
        return "none";
    case StopReason::CommandStop:
        return "command_stop";
    case StopReason::ModeChange:
        return "mode_change";
    case StopReason::LaneLost:
        return "lane_lost";
    case StopReason::LowConfidence:
        return "low_confidence";
    case StopReason::ServiceStop:
        return "service_stop";
    }
    return "unknown";
}

inline std::string_view toString(MotionCommand command) {
    switch (command) {
    case MotionCommand::Stopped:
        return "stopped";
    case MotionCommand::Forward:
        return "forward";
    }
    return "unknown";
}

} // namespace autonomous_car::services::autonomous_control
