#include "TestRegistry.hpp"
#include "domain/TrafficSignal.hpp"
#include "edge_impulse/LabelMapping.hpp"

namespace {

using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::edge_impulse::trafficSignalIdFromModelLabel;

void testCurrentEdgeImpulseLabelsMapToCanonicalIds() {
    const auto stop = trafficSignalIdFromModelLabel("Parada Obrigatoria sign");
    const auto turn_left = trafficSignalIdFromModelLabel("Vire a esquerda sign");

    expect(stop.has_value() && *stop == TrafficSignalId::Stop,
           "Label atual de parada deve mapear para stop.");
    expect(turn_left.has_value() && *turn_left == TrafficSignalId::TurnLeft,
           "Label atual de virar a esquerda deve mapear para turn_left.");
}

void testUnknownEdgeImpulseLabelsAreIgnored() {
    const auto unknown = trafficSignalIdFromModelLabel("Vire a direita sign");
    expect(!unknown.has_value(), "Labels ainda nao suportados devem ser ignorados.");
}

TestRegistrar label_mapping_supported_test("edge_impulse_label_mapping_maps_supported_labels",
                                           testCurrentEdgeImpulseLabelsMapToCanonicalIds);
TestRegistrar label_mapping_unknown_test("edge_impulse_label_mapping_ignores_unknown_labels",
                                         testUnknownEdgeImpulseLabelsAreIgnored);

} // namespace
