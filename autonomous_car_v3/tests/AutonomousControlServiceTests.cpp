#include <cmath>

#include "TestRegistry.hpp"
#include "pipeline/RoadSegmentationResult.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"

namespace {

using autonomous_car::DrivingMode;
using autonomous_car::services::autonomous_control::AutonomousControlService;
using autonomous_car::services::autonomous_control::StopReason;
using autonomous_car::services::autonomous_control::TrackingState;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using road_segmentation_lab::pipeline::LookaheadReference;
using road_segmentation_lab::pipeline::RoadSegmentationResult;

RoadSegmentationResult makeLaneResult(double near_error, double mid_error, double far_error,
                                      double confidence = 0.85, bool far_valid = true) {
    RoadSegmentationResult result;
    result.lane_found = true;
    result.confidence_score = confidence;
    result.near_reference = LookaheadReference{{160, 200}, 160, 220, 0.5, 0.0, near_error, 8, true};
    result.mid_reference = LookaheadReference{{160, 150}, 110, 170, 0.5, 0.0, mid_error, 8, true};
    result.far_reference =
        LookaheadReference{{160, 100}, 50, 120, 0.5, 0.0, far_error, 8, far_valid};
    result.heading_valid = true;
    result.curvature_valid = true;
    return result;
}

RoadSegmentationResult makeLaneMissingResult(double confidence = 0.1) {
    RoadSegmentationResult result;
    result.lane_found = false;
    result.confidence_score = confidence;
    return result;
}

void testZeroErrorKeepsNeutralSteering() {
    AutonomousControlService service;
    service.setDrivingMode(DrivingMode::Autonomous);
    service.startAutonomous();

    const auto snapshot = service.process(makeLaneResult(0.0, 0.0, 0.0), 1000);

    expect(snapshot.tracking_state == TrackingState::Tracking,
           "Com pista válida, o controlador deve entrar em tracking.");
    expect(std::abs(snapshot.steering_command) < 1e-6,
           "Erro zero deve produzir comando de direção neutro.");
}

void testSignedErrorsTurnToExpectedDirection() {
    AutonomousControlService positive_service;
    positive_service.setDrivingMode(DrivingMode::Autonomous);
    positive_service.startAutonomous();
    const auto positive = positive_service.process(makeLaneResult(0.5, 0.4, 0.3), 1000);

    AutonomousControlService negative_service;
    negative_service.setDrivingMode(DrivingMode::Autonomous);
    negative_service.startAutonomous();
    const auto negative = negative_service.process(makeLaneResult(-0.5, -0.4, -0.3), 1000);

    expect(positive.steering_command > 0.0,
           "Erro positivo deve resultar em correção para a direita.");
    expect(negative.steering_command < 0.0,
           "Erro negativo deve resultar em correção para a esquerda.");
}

void testFarReferenceInfluencesPreviewBeforeNearErrorGrows() {
    AutonomousControlService with_far_service;
    with_far_service.setDrivingMode(DrivingMode::Autonomous);
    with_far_service.startAutonomous();
    const auto with_far = with_far_service.process(makeLaneResult(0.0, 0.0, 0.8), 1000);

    AutonomousControlService no_far_service;
    no_far_service.setDrivingMode(DrivingMode::Autonomous);
    no_far_service.startAutonomous();
    const auto no_far = no_far_service.process(makeLaneResult(0.0, 0.0, 0.8, 0.85, false), 1000);

    expect(with_far.preview_error > 0.0,
           "Referencia far válida deve contribuir para o preview.");
    expect(with_far.steering_command > no_far.steering_command + 1e-6,
           "Far deve antecipar correção mesmo com near/mid neutros.");
}

void testLaneLossTriggersFailSafeAfterTolerance() {
    AutonomousControlService service;
    service.setDrivingMode(DrivingMode::Autonomous);
    service.startAutonomous();

    const auto tracking = service.process(makeLaneResult(0.4, 0.3, 0.2), 1000);
    const auto searching = service.process(makeLaneMissingResult(), 1150);
    const auto fail_safe = service.process(makeLaneMissingResult(), 1301);

    expect(tracking.tracking_state == TrackingState::Tracking,
           "Primeiro frame válido deve ficar em tracking.");
    expect(searching.tracking_state == TrackingState::Searching,
           "Perda momentânea de pista deve entrar em searching.");
    expect(searching.autonomous_started,
           "Durante a tolerância de perda de pista o start deve continuar latched.");
    expect(fail_safe.tracking_state == TrackingState::FailSafe,
           "Após o timeout a perda de pista deve ativar fail-safe.");
    expect(fail_safe.fail_safe_active, "Fail-safe deve ficar ativo.");
    expect(!fail_safe.autonomous_started,
           "Fail-safe deve desligar o start latched até novo comando.");
    expect(std::abs(fail_safe.steering_command) < 1e-6,
           "Fail-safe deve zerar a direção.");
}

void testStopAndModeChangeResetPidState() {
    AutonomousControlService service;
    service.setDrivingMode(DrivingMode::Autonomous);
    service.startAutonomous();
    const auto first = service.process(makeLaneResult(0.8, 0.6, 0.4), 1000);
    expect(std::abs(first.steering_command) > 0.05,
           "Primeiro tracking deve gerar correção perceptível.");

    service.stopAutonomous(StopReason::CommandStop);
    const auto stopped = service.snapshot();
    expect(!stopped.autonomous_started, "Stop manual deve desligar o start.");

    service.setDrivingMode(DrivingMode::Manual);
    const auto manual = service.snapshot();
    expect(manual.tracking_state == TrackingState::Manual,
           "Troca para manual deve refletir no estado.");

    service.setDrivingMode(DrivingMode::Autonomous);
    service.startAutonomous();
    const auto second = service.process(makeLaneResult(0.0, 0.0, 0.0), 2000);

    expect(std::abs(second.steering_command) < 1e-6,
           "Novo start após reset não deve carregar erro integral anterior.");
}

TestRegistrar zero_error_test("autonomous_control_zero_error_keeps_neutral_steering",
                              testZeroErrorKeepsNeutralSteering);
TestRegistrar signed_error_test("autonomous_control_signed_errors_turn_expected_direction",
                                testSignedErrorsTurnToExpectedDirection);
TestRegistrar far_preview_test("autonomous_control_far_reference_influences_preview",
                               testFarReferenceInfluencesPreviewBeforeNearErrorGrows);
TestRegistrar fail_safe_test("autonomous_control_lane_loss_triggers_fail_safe_after_tolerance",
                             testLaneLossTriggersFailSafeAfterTolerance);
TestRegistrar reset_test("autonomous_control_stop_and_mode_change_reset_pid_state",
                         testStopAndModeChangeResetPidState);

} // namespace
