#pragma once

#include <cstdint>
#include <mutex>

#include "controllers/PidController.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/autonomous_control/AutonomousControlTypes.hpp"

namespace autonomous_car::services::autonomous_control {

class AutonomousControlService {
public:
    AutonomousControlService();

    void updateConfig(const AutonomousControlConfig &config);
    [[nodiscard]] AutonomousControlConfig config() const;

    void setDrivingMode(DrivingMode mode);
    [[nodiscard]] DrivingMode drivingMode() const;

    void startAutonomous();
    void stopAutonomous(StopReason reason = StopReason::CommandStop);

    [[nodiscard]] AutonomousControlSnapshot process(
        const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
        std::int64_t timestamp_ms);

    [[nodiscard]] AutonomousControlSnapshot snapshot() const;

private:
    using RoadSegmentationResult = road_segmentation_lab::pipeline::RoadSegmentationResult;

    struct WeightedPreview {
        double error{0.0};
        ReferenceControlState near_reference;
        ReferenceControlState mid_reference;
        ReferenceControlState far_reference;
    };

    static std::int64_t currentTimestampMs();
    static double clampWeight(double value, double fallback);
    static double clampConfidence(double value);
    static std::vector<TrajectoryPoint> buildProjectedPath(double steering_command,
                                                           double preview_error);
    static bool isTrackingReady(const RoadSegmentationResult &result, double min_confidence);
    static WeightedPreview buildWeightedPreview(const RoadSegmentationResult &result,
                                                const AutonomousControlConfig &config);

    AutonomousControlSnapshot buildSnapshotForNoAutonomyLocked(std::int64_t timestamp_ms,
                                                               TrackingState state) const;
    AutonomousControlSnapshot buildStoppedSnapshotLocked(std::int64_t timestamp_ms) const;
    void resetPidLocked();
    void applyConfigToPidLocked();

    mutable std::mutex mutex_;
    AutonomousControlConfig config_;
    controllers::PidController pid_;
    DrivingMode driving_mode_{DrivingMode::Manual};
    bool autonomous_started_{false};
    bool fail_safe_active_{false};
    StopReason stop_reason_{StopReason::None};
    TrackingState tracking_state_{TrackingState::Manual};
    std::int64_t start_timestamp_ms_{0};
    std::int64_t last_tracking_timestamp_ms_{0};
    std::int64_t last_process_timestamp_ms_{0};
    double last_steering_command_{0.0};
    AutonomousControlSnapshot last_snapshot_;
};

} // namespace autonomous_car::services::autonomous_control
