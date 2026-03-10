#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "services/vision/VisionDebugStream.hpp"
#include "services/vision/VisionSubscriptionRegistry.hpp"

namespace {

using autonomous_car::services::vision::VisionDebugViewId;
using autonomous_car::services::vision::VisionSubscriptionRegistry;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::tests::expectContains;

void testVisionDebugSubscriptionParsing() {
    std::vector<std::string> invalid_tokens;
    const auto subscriptions =
        autonomous_car::services::vision::parseVisionDebugSubscriptionCsv(
            " raw,mask,annotated,unknown,dashboard ", &invalid_tokens);

    expect(subscriptions.size() == 4,
           "CSV deve carregar as quatro views validas sem duplicar.");
    expect(subscriptions.find(VisionDebugViewId::Raw) != subscriptions.end(),
           "CSV deve conter raw.");
    expect(subscriptions.find(VisionDebugViewId::Dashboard) != subscriptions.end(),
           "CSV deve conter dashboard.");
    expect(invalid_tokens.size() == 1 && invalid_tokens.front() == "unknown",
           "Tokens invalidos devem ser reportados.");
}

void testVisionFrameSerialization() {
    const cv::Mat frame(12, 20, CV_8UC3, cv::Scalar{32, 64, 96});
    const auto payload = autonomous_car::services::vision::buildVisionFrameJson(
        VisionDebugViewId::Mask, frame, 987654321, 75);

    expect(payload.has_value(), "Frame valido deve gerar payload JSON.");
    expectContains(*payload, "\"type\":\"vision.frame\"",
                   "Payload deve identificar vision.frame.");
    expectContains(*payload, "\"view\":\"mask\"",
                   "Payload deve carregar a view serializada.");
    expectContains(*payload, "\"timestamp_ms\":987654321",
                   "Payload deve serializar o timestamp.");
    expectContains(*payload, "\"mime\":\"image/jpeg\"",
                   "Payload deve anunciar JPEG.");
    expectContains(*payload, "\"width\":20", "Payload deve carregar width.");
    expectContains(*payload, "\"height\":12", "Payload deve carregar height.");
    expectContains(*payload, "\"data\":\"", "Payload deve conter o frame codificado.");
}

void testVisionSubscriptionRegistryTracksUnionAndCleanup() {
    VisionSubscriptionRegistry registry;
    registry.addSession(10);
    registry.addSession(20);

    registry.replaceSubscriptions(
        10, {VisionDebugViewId::Raw, VisionDebugViewId::Annotated});
    registry.replaceSubscriptions(20, {VisionDebugViewId::Mask});

    const auto first_union = registry.unionOfRequestedViews();
    expect(first_union.size() == 3,
           "Uniao inicial deve conter todas as views solicitadas.");
    expect(registry.hasSubscription(10, VisionDebugViewId::Annotated),
           "Sessao 10 deve manter annotated.");
    expect(!registry.hasSubscription(20, VisionDebugViewId::Dashboard),
           "Sessao 20 nao deve conter dashboard.");

    registry.removeSession(10);
    const auto second_union = registry.unionOfRequestedViews();
    expect(second_union.size() == 1 &&
               second_union.find(VisionDebugViewId::Mask) != second_union.end(),
           "Ao remover a sessao 10, a uniao deve refletir apenas a sessao restante.");
}

TestRegistrar subscription_parse_test("vision_debug_stream_subscription_parsing",
                                      testVisionDebugSubscriptionParsing);
TestRegistrar frame_serialization_test("vision_debug_stream_frame_serialization",
                                       testVisionFrameSerialization);
TestRegistrar registry_test("vision_subscription_registry_tracks_union_and_cleanup",
                            testVisionSubscriptionRegistryTracksUnionAndCleanup);

} // namespace
