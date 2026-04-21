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
    const auto turn_right = trafficSignalIdFromModelLabel("Vire a direita sign");

    expect(stop == TrafficSignalId::Stop,
           "Label atual de parada deve mapear para stop.");
    expect(turn_left == TrafficSignalId::TurnLeft,
           "Label atual de virar a esquerda deve mapear para turn_left.");
    expect(turn_right == TrafficSignalId::TurnRight,
           "Label atual de virar a direita deve mapear para turn_right.");
}

void testUnknownEdgeImpulseLabelsMapToUnknown() {
    const auto unknown = trafficSignalIdFromModelLabel("Vire em U sign");
    expect(unknown == TrafficSignalId::Unknown,
           "Labels ainda nao suportados devem cair em unknown.");
}

TestRegistrar label_mapping_supported_test("edge_impulse_label_mapping_maps_supported_labels",
                                           testCurrentEdgeImpulseLabelsMapToCanonicalIds);
TestRegistrar label_mapping_unknown_test("edge_impulse_label_mapping_maps_unknown_labels",
                                         testUnknownEdgeImpulseLabelsMapToUnknown);

} // namespace
