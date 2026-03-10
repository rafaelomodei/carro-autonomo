#include <string>

#include "TestRegistry.hpp"
#include "services/autonomous_control/AutonomousControlTelemetry.hpp"

namespace {

using autonomous_car::DrivingMode;
using autonomous_car::services::autonomous_control::AutonomousControlSnapshot;
using autonomous_car::services::autonomous_control::MotionCommand;
using autonomous_car::services::autonomous_control::StopReason;
using autonomous_car::services::autonomous_control::TrackingState;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expectContains;

void testAutonomousControlTelemetrySerialization() {
    AutonomousControlSnapshot snapshot;
    snapshot.driving_mode = DrivingMode::Autonomous;
    snapshot.autonomous_started = true;
    snapshot.fail_safe_active = false;
    snapshot.tracking_state = TrackingState::Tracking;
    snapshot.stop_reason = StopReason::None;
    snapshot.motion_command = MotionCommand::Forward;
    snapshot.lane_available = true;
    snapshot.confidence_ok = true;
    snapshot.confidence_score = 0.84;
    snapshot.preview_error = 0.12;
    snapshot.pid.error = 0.12;
    snapshot.pid.proportional = 0.05;
    snapshot.pid.integral = 0.01;
    snapshot.pid.derivative = 0.02;
    snapshot.pid.raw_output = 0.08;
    snapshot.pid.pid_output = 0.08;
    snapshot.steering_command = 0.06;
    snapshot.timestamp_ms = 222222;
    snapshot.near_reference.valid = true;
    snapshot.near_reference.used = true;
    snapshot.near_reference.configured_weight = 0.5;
    snapshot.near_reference.applied_weight = 0.5;
    snapshot.projected_path.push_back({0.0, 0.0});
    snapshot.projected_path.push_back({0.1, 0.5});

    const std::string json =
        autonomous_car::services::autonomous_control::buildAutonomousControlTelemetryJson(
            snapshot);

    expectContains(json, "\"type\":\"telemetry.autonomous_control\"",
                   "Payload deve identificar a telemetria autonoma.");
    expectContains(json, "\"timestamp_ms\":222222",
                   "Payload deve serializar timestamp.");
    expectContains(json, "\"driving_mode\":\"autonomous\"",
                   "Payload deve serializar o modo.");
    expectContains(json, "\"tracking_state\":\"tracking\"",
                   "Payload deve serializar o estado.");
    expectContains(json, "\"motion_command\":\"forward\"",
                   "Payload deve serializar o comando de movimento.");
    expectContains(json, "\"references\":{\"near\":",
                   "Payload deve conter a seção de referências.");
    expectContains(json, "\"projected_path\":[{",
                   "Payload deve conter a trajetória projetada.");
}

TestRegistrar telemetry_test("autonomous_control_telemetry_serialization",
                             testAutonomousControlTelemetrySerialization);

} // namespace
