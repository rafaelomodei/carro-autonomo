#include "services/autonomous_control/AutonomousControlService.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace autonomous_car::services::autonomous_control {
namespace {

constexpr double kFallbackDtSeconds = 1.0 / 30.0;
constexpr double kPreviewPointClamp = 1.0;
constexpr int kProjectedPointCount = 12;

double clamp01(double value) { return std::clamp(value, 0.0, 1.0); }

} // namespace

AutonomousControlService::AutonomousControlService() {
    std::lock_guard<std::mutex> lock(mutex_);
    applyConfigToPidLocked();
    last_snapshot_ = buildSnapshotForNoAutonomyLocked(0, TrackingState::Manual);
}

void AutonomousControlService::updateConfig(const AutonomousControlConfig &config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    config_.pid_output_limit = std::max(std::abs(config.pid_output_limit), 0.01);
    config_.preview_near_weight = clampWeight(config.preview_near_weight, 0.5);
    config_.preview_mid_weight = clampWeight(config.preview_mid_weight, 0.3);
    config_.preview_far_weight = clampWeight(config.preview_far_weight, 0.2);
    config_.max_steering_delta_per_update =
        std::max(std::abs(config.max_steering_delta_per_update), 0.0);
    config_.min_confidence = clampConfidence(config.min_confidence);
    config_.lane_loss_timeout_ms = std::max(config.lane_loss_timeout_ms, 0);
    applyConfigToPidLocked();
}

AutonomousControlConfig AutonomousControlService::config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void AutonomousControlService::setDrivingMode(DrivingMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (driving_mode_ == mode) {
        return;
    }

    driving_mode_ = mode;
    resetPidLocked();
    if (mode == DrivingMode::Manual) {
        autonomous_started_ = false;
        fail_safe_active_ = false;
        tracking_state_ = TrackingState::Manual;
        stop_reason_ = StopReason::ModeChange;
    } else {
        autonomous_started_ = false;
        fail_safe_active_ = false;
        tracking_state_ = TrackingState::Idle;
        stop_reason_ = StopReason::None;
    }
    start_timestamp_ms_ = 0;
    last_tracking_timestamp_ms_ = 0;
    last_steering_command_ = 0.0;
    last_snapshot_ =
        buildSnapshotForNoAutonomyLocked(currentTimestampMs(), tracking_state_);
}

DrivingMode AutonomousControlService::drivingMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return driving_mode_;
}

void AutonomousControlService::startAutonomous() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (driving_mode_ != DrivingMode::Autonomous) {
        return;
    }
    autonomous_started_ = true;
    fail_safe_active_ = false;
    stop_reason_ = StopReason::None;
    tracking_state_ = TrackingState::Searching;
    start_timestamp_ms_ = currentTimestampMs();
    last_tracking_timestamp_ms_ = 0;
    last_steering_command_ = 0.0;
    resetPidLocked();
    last_snapshot_ = buildSnapshotForNoAutonomyLocked(start_timestamp_ms_, TrackingState::Searching);
    last_snapshot_.autonomous_started = true;
}

void AutonomousControlService::stopAutonomous(StopReason reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    autonomous_started_ = false;
    fail_safe_active_ =
        reason == StopReason::LaneLost || reason == StopReason::LowConfidence;
    stop_reason_ = reason;
    tracking_state_ =
        driving_mode_ == DrivingMode::Manual ? TrackingState::Manual
                                             : (fail_safe_active_ ? TrackingState::FailSafe
                                                                  : TrackingState::Idle);
    start_timestamp_ms_ = 0;
    last_steering_command_ = 0.0;
    resetPidLocked();
    last_snapshot_ =
        buildSnapshotForNoAutonomyLocked(currentTimestampMs(), tracking_state_);
    last_snapshot_.stop_reason = stop_reason_;
    last_snapshot_.fail_safe_active = fail_safe_active_;
}

AutonomousControlSnapshot AutonomousControlService::process(const RoadSegmentationResult &result,
                                                            std::int64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (timestamp_ms <= 0) {
        timestamp_ms = currentTimestampMs();
    }

    if (driving_mode_ == DrivingMode::Manual) {
        last_snapshot_ = buildSnapshotForNoAutonomyLocked(timestamp_ms, TrackingState::Manual);
        last_process_timestamp_ms_ = timestamp_ms;
        return last_snapshot_;
    }

    if (!autonomous_started_) {
        const TrackingState idle_state =
            fail_safe_active_ ? TrackingState::FailSafe : TrackingState::Idle;
        last_snapshot_ = buildSnapshotForNoAutonomyLocked(timestamp_ms, idle_state);
        last_process_timestamp_ms_ = timestamp_ms;
        return last_snapshot_;
    }

    AutonomousControlSnapshot snapshot;
    snapshot.driving_mode = driving_mode_;
    snapshot.autonomous_started = autonomous_started_;
    snapshot.fail_safe_active = fail_safe_active_;
    snapshot.tracking_state = tracking_state_;
    snapshot.stop_reason = stop_reason_;
    snapshot.timestamp_ms = timestamp_ms;
    snapshot.heading_error_rad = result.heading_error_rad;
    snapshot.heading_valid = result.heading_valid;
    snapshot.curvature_indicator_rad = result.curvature_indicator_rad;
    snapshot.curvature_valid = result.curvature_valid;
    snapshot.confidence_score = result.confidence_score;
    snapshot.confidence_ok = result.confidence_score >= config_.min_confidence;

    const bool tracking_ready = isTrackingReady(result, config_.min_confidence);
    const std::int64_t reference_timestamp =
        last_tracking_timestamp_ms_ > 0 ? last_tracking_timestamp_ms_ : start_timestamp_ms_;
    const std::int64_t elapsed_without_tracking =
        reference_timestamp > 0 ? std::max<std::int64_t>(0, timestamp_ms - reference_timestamp) : 0;

    if (!tracking_ready) {
        if (config_.lane_loss_timeout_ms > 0 &&
            elapsed_without_tracking <= config_.lane_loss_timeout_ms) {
            snapshot.tracking_state = TrackingState::Searching;
            snapshot.motion_command = MotionCommand::Forward;
            snapshot.steering_command = last_steering_command_;
            snapshot.projected_path =
                buildProjectedPath(snapshot.steering_command, snapshot.preview_error);
            snapshot.last_tracking_timestamp_ms = last_tracking_timestamp_ms_;
            last_snapshot_ = snapshot;
            last_process_timestamp_ms_ = timestamp_ms;
            return last_snapshot_;
        }

        stop_reason_ = snapshot.confidence_ok ? StopReason::LaneLost : StopReason::LowConfidence;
        autonomous_started_ = false;
        fail_safe_active_ = true;
        tracking_state_ = TrackingState::FailSafe;
        snapshot = buildStoppedSnapshotLocked(timestamp_ms);
        snapshot.fail_safe_active = true;
        snapshot.tracking_state = TrackingState::FailSafe;
        snapshot.stop_reason = stop_reason_;
        snapshot.confidence_score = result.confidence_score;
        snapshot.confidence_ok = result.confidence_score >= config_.min_confidence;
        snapshot.heading_error_rad = result.heading_error_rad;
        snapshot.heading_valid = result.heading_valid;
        snapshot.curvature_indicator_rad = result.curvature_indicator_rad;
        snapshot.curvature_valid = result.curvature_valid;
        last_snapshot_ = snapshot;
        last_process_timestamp_ms_ = timestamp_ms;
        return last_snapshot_;
    }

    WeightedPreview preview = buildWeightedPreview(result, config_);
    snapshot.preview_error = preview.error;
    snapshot.near_reference = preview.near_reference;
    snapshot.mid_reference = preview.mid_reference;
    snapshot.far_reference = preview.far_reference;
    snapshot.lane_available = true;
    snapshot.confidence_ok = true;
    snapshot.tracking_state = TrackingState::Tracking;
    snapshot.stop_reason = StopReason::None;
    snapshot.motion_command = MotionCommand::Forward;

    double dt_seconds = kFallbackDtSeconds;
    if (last_process_timestamp_ms_ > 0 && timestamp_ms > last_process_timestamp_ms_) {
        dt_seconds =
            static_cast<double>(timestamp_ms - last_process_timestamp_ms_) / 1000.0;
    }

    if (last_process_timestamp_ms_ > 0 && config_.lane_loss_timeout_ms > 0 &&
        timestamp_ms - last_process_timestamp_ms_ > config_.lane_loss_timeout_ms) {
        resetPidLocked();
        dt_seconds = kFallbackDtSeconds;
    }

    const controllers::PidComputation pid_computation = pid_.compute(snapshot.preview_error, dt_seconds);
    snapshot.pid.error = pid_computation.error;
    snapshot.pid.proportional = pid_computation.proportional;
    snapshot.pid.integral = pid_computation.integral;
    snapshot.pid.derivative = pid_computation.derivative;
    snapshot.pid.raw_output = pid_computation.raw_output;
    snapshot.pid.pid_output = pid_computation.output;

    double steering_command = pid_computation.output;
    const double max_delta = config_.max_steering_delta_per_update;
    if (max_delta > 0.0) {
        const double min_allowed = last_steering_command_ - max_delta;
        const double max_allowed = last_steering_command_ + max_delta;
        steering_command = std::clamp(steering_command, min_allowed, max_allowed);
    }

    steering_command =
        std::clamp(steering_command, -config_.pid_output_limit, config_.pid_output_limit);

    snapshot.steering_command = steering_command;
    snapshot.projected_path = buildProjectedPath(snapshot.steering_command, snapshot.preview_error);
    snapshot.last_tracking_timestamp_ms = timestamp_ms;

    autonomous_started_ = true;
    fail_safe_active_ = false;
    tracking_state_ = TrackingState::Tracking;
    stop_reason_ = StopReason::None;
    last_tracking_timestamp_ms_ = timestamp_ms;
    last_process_timestamp_ms_ = timestamp_ms;
    last_steering_command_ = steering_command;
    last_snapshot_ = snapshot;
    return last_snapshot_;
}

AutonomousControlSnapshot AutonomousControlService::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_snapshot_;
}

std::int64_t AutonomousControlService::currentTimestampMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

double AutonomousControlService::clampWeight(double value, double fallback) {
    if (!std::isfinite(value) || value < 0.0) {
        return fallback;
    }
    return value;
}

double AutonomousControlService::clampConfidence(double value) {
    if (!std::isfinite(value)) {
        return 0.25;
    }
    return clamp01(value);
}

std::vector<TrajectoryPoint> AutonomousControlService::buildProjectedPath(double steering_command,
                                                                          double preview_error) {
    std::vector<TrajectoryPoint> projected_path;
    projected_path.reserve(kProjectedPointCount);

    const double clamped_steering =
        std::clamp(steering_command, -kPreviewPointClamp, kPreviewPointClamp);
    const double clamped_error = std::clamp(preview_error, -kPreviewPointClamp, kPreviewPointClamp);

    for (int index = 0; index < kProjectedPointCount; ++index) {
        const double y = static_cast<double>(index) / static_cast<double>(kProjectedPointCount - 1);
        const double linear_component = clamped_error * 0.22 * y;
        const double curved_component = clamped_steering * 0.85 * y * y;
        projected_path.push_back(TrajectoryPoint{
            std::clamp(linear_component + curved_component, -kPreviewPointClamp, kPreviewPointClamp),
            y,
        });
    }

    return projected_path;
}

bool AutonomousControlService::isTrackingReady(const RoadSegmentationResult &result,
                                               double min_confidence) {
    return result.lane_found && result.confidence_score >= min_confidence &&
           result.near_reference.valid && result.mid_reference.valid;
}

AutonomousControlService::WeightedPreview AutonomousControlService::buildWeightedPreview(
    const RoadSegmentationResult &result, const AutonomousControlConfig &config) {
    WeightedPreview preview;

    preview.near_reference.valid = result.near_reference.valid;
    preview.near_reference.configured_weight = config.preview_near_weight;
    preview.near_reference.steering_error_normalized =
        result.near_reference.steering_error_normalized;
    preview.near_reference.lateral_offset_px = result.near_reference.lateral_offset_px;

    preview.mid_reference.valid = result.mid_reference.valid;
    preview.mid_reference.configured_weight = config.preview_mid_weight;
    preview.mid_reference.steering_error_normalized =
        result.mid_reference.steering_error_normalized;
    preview.mid_reference.lateral_offset_px = result.mid_reference.lateral_offset_px;

    preview.far_reference.valid = result.far_reference.valid;
    preview.far_reference.configured_weight = config.preview_far_weight;
    preview.far_reference.steering_error_normalized =
        result.far_reference.steering_error_normalized;
    preview.far_reference.lateral_offset_px = result.far_reference.lateral_offset_px;

    double total_weight = 0.0;
    if (result.near_reference.valid) {
        total_weight += config.preview_near_weight;
    }
    if (result.mid_reference.valid) {
        total_weight += config.preview_mid_weight;
    }
    if (result.far_reference.valid) {
        total_weight += config.preview_far_weight;
    }

    if (total_weight <= 0.0) {
        return preview;
    }

    if (result.near_reference.valid) {
        preview.near_reference.used = true;
        preview.near_reference.applied_weight = config.preview_near_weight / total_weight;
        preview.error += preview.near_reference.steering_error_normalized *
                         preview.near_reference.applied_weight;
    }
    if (result.mid_reference.valid) {
        preview.mid_reference.used = true;
        preview.mid_reference.applied_weight = config.preview_mid_weight / total_weight;
        preview.error += preview.mid_reference.steering_error_normalized *
                         preview.mid_reference.applied_weight;
    }
    if (result.far_reference.valid) {
        preview.far_reference.used = true;
        preview.far_reference.applied_weight = config.preview_far_weight / total_weight;
        preview.error += preview.far_reference.steering_error_normalized *
                         preview.far_reference.applied_weight;
    }

    return preview;
}

AutonomousControlSnapshot AutonomousControlService::buildSnapshotForNoAutonomyLocked(
    std::int64_t timestamp_ms, TrackingState state) const {
    AutonomousControlSnapshot snapshot;
    snapshot.driving_mode = driving_mode_;
    snapshot.autonomous_started = autonomous_started_;
    snapshot.fail_safe_active = fail_safe_active_;
    snapshot.tracking_state = state;
    snapshot.stop_reason = stop_reason_;
    snapshot.motion_command = MotionCommand::Stopped;
    snapshot.timestamp_ms = timestamp_ms;
    snapshot.last_tracking_timestamp_ms = last_tracking_timestamp_ms_;
    snapshot.projected_path = buildProjectedPath(0.0, 0.0);
    return snapshot;
}

AutonomousControlSnapshot AutonomousControlService::buildStoppedSnapshotLocked(
    std::int64_t timestamp_ms) const {
    AutonomousControlSnapshot snapshot =
        buildSnapshotForNoAutonomyLocked(timestamp_ms, TrackingState::FailSafe);
    snapshot.driving_mode = driving_mode_;
    snapshot.fail_safe_active = fail_safe_active_;
    snapshot.stop_reason = stop_reason_;
    return snapshot;
}

void AutonomousControlService::resetPidLocked() {
    pid_.reset();
    last_tracking_timestamp_ms_ = 0;
    last_process_timestamp_ms_ = 0;
}

void AutonomousControlService::applyConfigToPidLocked() {
    pid_.setCoefficients(config_.pid_kp, config_.pid_ki, config_.pid_kd);
    pid_.setOutputLimits(-config_.pid_output_limit, config_.pid_output_limit);
    pid_.setIntegralLimits(-config_.pid_output_limit, config_.pid_output_limit);
}

} // namespace autonomous_car::services::autonomous_control
