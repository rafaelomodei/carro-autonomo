#include "TestRegistry.hpp"
#include "policy/DetectionPolicy.hpp"

namespace {

using traffic_sign_service::SignalDetection;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::policy::DetectionPolicy;
using traffic_sign_service::policy::DetectionPolicyConfig;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

SignalDetection makeDetection(TrafficSignalId signal_id, double confidence) {
    SignalDetection detection;
    detection.signal_id = signal_id;
    detection.confidence = confidence;
    return detection;
}

void testPolicyRequiresStableConsecutiveFrames() {
    DetectionPolicy policy(DetectionPolicyConfig{0.60, 3, 2000});

    expect(!policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.92), 1000).has_value(),
           "Primeiro frame nao deve emitir evento.");
    expect(!policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.91), 1100).has_value(),
           "Segundo frame ainda nao deve emitir evento.");
    const auto emitted = policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.90), 1200);

    expect(emitted.has_value() && *emitted == TrafficSignalId::Stop,
           "Terceiro frame consecutivo deve emitir o sinal confirmado.");
}

void testPolicyDebouncesAndRequiresReappearanceAfterCooldown() {
    DetectionPolicy policy(DetectionPolicyConfig{0.60, 3, 2000});

    policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 1000);
    policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 1100);
    expect(policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 1200).has_value(),
           "Primeira aparicao estavel deve emitir.");

    expect(!policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 3400).has_value(),
           "Mesma sequencia nao deve reenviar mesmo depois de muito tempo.");

    policy.evaluate(std::nullopt, 3500);

    policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 3600);
    policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 3700);
    expect(policy.evaluate(makeDetection(TrafficSignalId::Stop, 0.9), 3800).has_value(),
           "Reaparicao apos reset e cooldown deve gerar novo evento.");
}

TestRegistrar detection_policy_stable_test("detection_policy_requires_consecutive_frames",
                                           testPolicyRequiresStableConsecutiveFrames);
TestRegistrar detection_policy_cooldown_test(
    "detection_policy_debounces_and_rearms_after_reappearance",
    testPolicyDebouncesAndRequiresReappearanceAfterCooldown);

} // namespace
