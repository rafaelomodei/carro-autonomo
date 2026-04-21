#include "TestRegistry.hpp"
#include "services/websocket/WebSocketProtocol.hpp"

namespace {

using autonomous_car::services::websocket::MessageChannel;
using autonomous_car::services::websocket::messageRequiresControllerRole;
using autonomous_car::services::websocket::parseInboundMessage;
using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;

void testSignalMessagesParseWithoutControlRole() {
    const auto parsed = parseInboundMessage("signal:detected=stop");

    expect(parsed.has_value(), "Mensagem signal valida deve ser parseada.");
    expect(parsed->channel == MessageChannel::Signal,
           "Mensagem signal deve cair no canal dedicado.");
    expect(parsed->key == "detected", "Chave do evento de sinalizacao deve ser detected.");
    expect(parsed->value.has_value() && *parsed->value == "stop",
           "Valor do evento deve carregar o signal_id.");
    expect(!messageRequiresControllerRole(parsed->channel),
           "Mensagens signal nao devem exigir papel control.");
}

void testInvalidSignalMessageIsRejected() {
    const auto parsed = parseInboundMessage("signal:detected=");
    expect(!parsed.has_value(), "Mensagens signal sem payload devem ser rejeitadas.");
}

TestRegistrar websocket_signal_parse_test("websocket_protocol_parses_signal_detected_messages",
                                          testSignalMessagesParseWithoutControlRole);
TestRegistrar websocket_signal_invalid_test("websocket_protocol_rejects_empty_signal_payload",
                                            testInvalidSignalMessageIsRejected);

} // namespace
