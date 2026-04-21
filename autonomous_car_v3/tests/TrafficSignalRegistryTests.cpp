#include "TestRegistry.hpp"
#include "services/traffic_signals/TrafficSignalRegistry.hpp"

namespace {

using autonomous_car::services::traffic_signals::TrafficSignalId;
using autonomous_car::services::traffic_signals::TrafficSignalRegistry;
using autonomous_car::services::traffic_signals::toString;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;

void testRegistryStoresCanonicalSignal() {
    TrafficSignalRegistry registry;

    expect(registry.recordDetectedSignal("stop", 1234),
           "Registry deve aceitar o sinal canonical stop.");

    const auto last_signal = registry.lastDetectedSignal();
    expect(last_signal.has_value(), "Registry deve armazenar o ultimo sinal recebido.");
    expect(last_signal->signal_id == TrafficSignalId::Stop,
           "Registry deve salvar o enum canonical correspondente.");
    expect(last_signal->received_at_ms == 1234,
           "Registry deve preservar o timestamp do evento.");
}

void testRegistryRejectsUnknownSignal() {
    TrafficSignalRegistry registry;
    registry.recordDetectedSignal(TrafficSignalId::TurnLeft, 42);

    expect(!registry.recordDetectedSignal("parking", 84),
           "Registry nao deve aceitar sinais fora do contrato atual.");

    const auto last_signal = registry.lastDetectedSignal();
    expect(last_signal.has_value(), "Registry deve manter o ultimo sinal valido.");
    expect(last_signal->signal_id == TrafficSignalId::TurnLeft,
           "Sinal invalido nao pode sobrescrever o ultimo evento valido.");
    expect(toString(last_signal->signal_id) == "turn_left",
           "Enum armazenado deve continuar refletindo o ultimo sinal valido.");
}

TestRegistrar traffic_signal_store_test("traffic_signal_registry_stores_last_event",
                                        testRegistryStoresCanonicalSignal);
TestRegistrar traffic_signal_reject_test("traffic_signal_registry_rejects_unknown_signals",
                                         testRegistryRejectsUnknownSignal);

} // namespace
