#include <algorithm>
#include <vector>

#include "TestRegistry.hpp"
#include "services/websocket/ClientRegistry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::services::websocket::ClientRegistry;
using autonomous_car::services::websocket::ClientRole;

void testRegisteredControlAndSingleControllerRule() {
    ClientRegistry registry;
    registry.addSession(1);
    registry.addSession(2);

    expect(registry.requestRole(1, ClientRole::Control),
           "Primeiro cliente deve conseguir assumir papel control.");
    expect(registry.roleOf(1) == ClientRole::Control,
           "Cliente 1 deve ser control.");
    expect(!registry.requestRole(2, ClientRole::Control),
           "Segundo cliente nao deve assumir control com control ativo.");
    expect(registry.roleOf(2) == ClientRole::Telemetry,
           "Cliente 2 deve cair como telemetry.");

    registry.removeSession(1);
    expect(!registry.controllerId().has_value(),
           "Ao remover o controlador, nao deve restar control ativo.");
    expect(!registry.ensureController(2),
           "Cliente marcado como telemetry nao deve virar control implicitamente.");
}

void testImplicitControlCompatibilityAndReconnect() {
    ClientRegistry registry;
    registry.addSession(10);
    registry.addSession(20);

    expect(registry.ensureController(10),
           "Primeiro cliente sem papel explicito deve assumir control ao enviar command/config.");
    expect(registry.roleOf(10) == ClientRole::Control,
           "Cliente 10 deve virar control.");
    expect(!registry.ensureController(20),
           "Segundo cliente deve cair como telemetry quando ja houver control.");
    expect(registry.roleOf(20) == ClientRole::Telemetry,
           "Cliente 20 deve ficar como telemetry.");

    const auto first_snapshot = registry.sessionIds();
    expect(first_snapshot.size() == 2, "Dois clientes devem estar registrados.");

    registry.removeSession(20);
    registry.addSession(30);
    const auto second_snapshot = registry.sessionIds();
    expect(second_snapshot.size() == 2,
           "Reconexao deve manter o conjunto de sessoes atualizado.");
    expect(std::find(second_snapshot.begin(), second_snapshot.end(), 30) != second_snapshot.end(),
           "Novo cliente reconnectado deve aparecer nas sessoes ativas.");
}

TestRegistrar websocket_control_test("websocket_registered_control_and_single_controller_rule",
                                     testRegisteredControlAndSingleControllerRule);
TestRegistrar websocket_compat_test("websocket_implicit_control_compatibility_and_reconnect",
                                    testImplicitControlCompatibilityAndReconnect);

} // namespace
