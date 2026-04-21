#include "TestRegistry.hpp"
#include "policy/DetectionPolicy.hpp"

namespace {

using traffic_sign_service::SignalDetection;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::TrafficSignDetectorState;
using traffic_sign_service::TrafficSignFrameResult;
using traffic_sign_service::policy::DetectionPolicy;
using traffic_sign_service::policy::DetectionPolicyConfig;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

SignalDetection makeDetection(TrafficSignalId signal_id, double confidence) {
    SignalDetection detection;
    detection.signal_id = signal_id;
    detection.confidence = confidence;
    detection.display_label = traffic_sign_service::displayLabel(signal_id);
    return detection;
}

TrafficSignFrameResult makeFrameResult(std::optional<SignalDetection> active_detection) {
    TrafficSignFrameResult result;
    result.detector_state = active_detection.has_value() ? TrafficSignDetectorState::Confirmed
                                                         : TrafficSignDetectorState::Idle;
    result.active_detection = std::move(active_detection);
    return result;
}

void testPolicyEmitsOnlyOnceWhileSignalRemainsActive() {
    DetectionPolicy policy(DetectionPolicyConfig{2000});

    expect(policy
               .evaluate(makeFrameResult(makeDetection(TrafficSignalId::Stop, 0.92)), 1000)
               .has_value(),
           "Primeiro estado confirmado deve emitir evento.");
    expect(!policy
                .evaluate(makeFrameResult(makeDetection(TrafficSignalId::Stop, 0.91)), 1100)
                .has_value(),
           "Mesmo sinal ativo em seguida nao deve emitir novamente.");
}

void testPolicyRequiresSignalToDisappearBeforeCooldownRearms() {
    DetectionPolicy policy(DetectionPolicyConfig{2000});

    expect(policy
               .evaluate(makeFrameResult(makeDetection(TrafficSignalId::Stop, 0.90)), 1000)
               .has_value(),
           "Primeira aparicao confirmada deve emitir.");

    expect(!policy
                .evaluate(makeFrameResult(makeDetection(TrafficSignalId::Stop, 0.90)), 3400)
                .has_value(),
           "Sem desaparecer, o mesmo sinal nao deve ser reenviado.");

    policy.evaluate(makeFrameResult(std::nullopt), 3500);

    expect(policy
               .evaluate(makeFrameResult(makeDetection(TrafficSignalId::Stop, 0.90)), 3800)
               .has_value(),
           "Reaparicao apos reset e cooldown deve gerar novo evento.");
}

TestRegistrar detection_policy_active_test(
    "detection_policy_emits_only_once_while_signal_is_active",
    testPolicyEmitsOnlyOnceWhileSignalRemainsActive);
TestRegistrar detection_policy_rearm_test(
    "detection_policy_rearms_after_signal_disappears_and_cooldown_passes",
    testPolicyRequiresSignalToDisappearBeforeCooldownRearms);

} // namespace
