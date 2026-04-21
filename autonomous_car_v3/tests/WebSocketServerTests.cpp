#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "TestRegistry.hpp"
#include "controllers/CommandRouter.hpp"
#include "services/WebSocketServer.hpp"
#include "services/websocket/ClientRegistry.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::services::websocket::ClientRegistry;
using autonomous_car::services::websocket::ClientRole;

bool sendAll(int socket_fd, const void *buffer, size_t length) {
    const auto *bytes = static_cast<const unsigned char *>(buffer);
    size_t sent_total = 0;
    while (sent_total < length) {
        const ssize_t sent = send(socket_fd, bytes + sent_total, length - sent_total, 0);
        if (sent <= 0) {
            return false;
        }
        sent_total += static_cast<size_t>(sent);
    }
    return true;
}

std::optional<std::string> readHttpResponse(int socket_fd) {
    std::string response;
    std::array<char, 1024> buffer{};
    while (response.find("\r\n\r\n") == std::string::npos) {
        const ssize_t bytes = recv(socket_fd, buffer.data(), buffer.size(), 0);
        if (bytes <= 0) {
            return std::nullopt;
        }
        response.append(buffer.data(), static_cast<size_t>(bytes));
    }
    return response;
}

bool sendClientTextFrame(int socket_fd, const std::string &payload) {
    std::vector<unsigned char> frame;
    frame.reserve(payload.size() + 16);
    frame.push_back(0x81);

    const size_t payload_length = payload.size();
    if (payload_length <= 125) {
        frame.push_back(static_cast<unsigned char>(0x80u | payload_length));
    } else if (payload_length <= 0xFFFFu) {
        frame.push_back(0x80u | 126u);
        frame.push_back(static_cast<unsigned char>((payload_length >> 8) & 0xFFu));
        frame.push_back(static_cast<unsigned char>(payload_length & 0xFFu));
    } else {
        frame.push_back(0x80u | 127u);
        for (int shift = 56; shift >= 0; shift -= 8) {
            frame.push_back(static_cast<unsigned char>((payload_length >> shift) & 0xFFu));
        }
    }

    const std::array<unsigned char, 4> mask = {0x12, 0x34, 0x56, 0x78};
    frame.insert(frame.end(), mask.begin(), mask.end());
    for (size_t index = 0; index < payload.size(); ++index) {
        frame.push_back(static_cast<unsigned char>(payload[index]) ^ mask[index % mask.size()]);
    }

    return sendAll(socket_fd, frame.data(), frame.size());
}

std::optional<std::string> readServerTextFrame(int socket_fd, int timeout_ms) {
    timeval timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    unsigned char header[2];
    const ssize_t header_bytes = recv(socket_fd, header, sizeof(header), 0);
    if (header_bytes <= 0) {
        return std::nullopt;
    }
    if (header_bytes != 2) {
        return std::nullopt;
    }

    const unsigned char opcode = header[0] & 0x0F;
    if (opcode == 0x8) {
        return std::nullopt;
    }

    std::uint64_t payload_length = header[1] & 0x7F;
    if (payload_length == 126) {
        unsigned char extended[2];
        if (recv(socket_fd, extended, sizeof(extended), MSG_WAITALL) != 2) {
            return std::nullopt;
        }
        payload_length = (static_cast<std::uint64_t>(extended[0]) << 8) | extended[1];
    } else if (payload_length == 127) {
        unsigned char extended[8];
        if (recv(socket_fd, extended, sizeof(extended), MSG_WAITALL) != 8) {
            return std::nullopt;
        }
        payload_length = 0;
        for (int i = 0; i < 8; ++i) {
            payload_length = (payload_length << 8) | extended[i];
        }
    }

    std::string payload(static_cast<size_t>(payload_length), '\0');
    if (recv(socket_fd, payload.data(), static_cast<size_t>(payload_length), MSG_WAITALL) !=
        static_cast<ssize_t>(payload_length)) {
        return std::nullopt;
    }

    return payload;
}

int reserveFreePort() {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Falha ao criar socket para reservar porta.");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(fd);
        throw std::runtime_error("Falha ao reservar porta local.");
    }

    socklen_t address_len = sizeof(address);
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&address), &address_len) < 0) {
        close(fd);
        throw std::runtime_error("Falha ao consultar porta reservada.");
    }

    const int port = ntohs(address.sin_port);
    close(fd);
    return port;
}

class TestWebSocketClient {
public:
    TestWebSocketClient(const std::string &host, int port) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error("Falha ao criar socket cliente.");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(port));
        if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
            close(fd_);
            throw std::runtime_error("Falha ao converter endereco do cliente.");
        }

        if (connect(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
            close(fd_);
            throw std::runtime_error(std::string("Falha ao conectar cliente: ") +
                                     std::strerror(errno));
        }

        const std::string request =
            "GET / HTTP/1.1\r\n"
            "Host: " + host + ":" + std::to_string(port) + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGVzdC1jb2RleC1rZXk=\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n";
        if (!sendAll(fd_, request.data(), request.size())) {
            close(fd_);
            throw std::runtime_error("Falha ao enviar handshake WebSocket.");
        }

        const auto response = readHttpResponse(fd_);
        if (!response || response->find("101 Switching Protocols") == std::string::npos) {
            close(fd_);
            throw std::runtime_error("Handshake WebSocket nao foi aceito.");
        }
    }

    ~TestWebSocketClient() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    void sendText(const std::string &payload) {
        if (!sendClientTextFrame(fd_, payload)) {
            throw std::runtime_error("Falha ao enviar frame de texto.");
        }
    }

    std::optional<std::string> readText(int timeout_ms) {
        return readServerTextFrame(fd_, timeout_ms);
    }

private:
    int fd_{-1};
};

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

void testServerRelaysExternalTrafficSignTelemetryAndKeepsSignalChannel() {
    const int port = reserveFreePort();
    autonomous_car::controllers::CommandRouter command_router;
    std::vector<std::string> received_signals;

    autonomous_car::services::WebSocketServer server(
        "127.0.0.1", port, command_router, [](const std::string &, const std::string &) {
            return true;
        },
        [] { return autonomous_car::DrivingMode::Manual; },
        [&received_signals](const std::string &signal_id) {
            received_signals.push_back(signal_id);
            return true;
        });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    TestWebSocketClient relay_client("127.0.0.1", port);
    TestWebSocketClient consumer_client("127.0.0.1", port);
    relay_client.sendText("client:telemetry");
    consumer_client.sendText("client:telemetry");

    const std::string telemetry_payload =
        R"({"type":"telemetry.traffic_sign_detection","timestamp_ms":1,"source":"WebSocket","detector_state":"confirmed","roi":{"left_ratio":0.55,"right_ratio":1.0,"right_width_ratio":0.45,"top_ratio":0.08,"bottom_ratio":0.72},"raw_detections":[],"candidate":null,"active_detection":null,"last_error":""})";
    relay_client.sendText(telemetry_payload);

    const auto relayed_payload = consumer_client.readText(1000);
    expect(relayed_payload.has_value(),
           "Cliente consumidor deve receber a telemetria externa relayada.");
    expect(*relayed_payload == telemetry_payload,
           "Servidor deve redistribuir o payload bruto sem reserializar.");

    relay_client.sendText("signal:detected=stop");
    const auto signal_ready_deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (received_signals.empty() && std::chrono::steady_clock::now() < signal_ready_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.stop();

    expect(received_signals.size() == 1 && received_signals.front() == "stop",
           "Canal signal:detected deve continuar funcional em paralelo ao relay JSON.");
}

TestRegistrar websocket_control_test("websocket_registered_control_and_single_controller_rule",
                                     testRegisteredControlAndSingleControllerRule);
TestRegistrar websocket_compat_test("websocket_implicit_control_compatibility_and_reconnect",
                                    testImplicitControlCompatibilityAndReconnect);
TestRegistrar websocket_external_telemetry_test(
    "websocket_server_relays_external_traffic_sign_telemetry_and_keeps_signal_channel",
    testServerRelaysExternalTrafficSignTelemetryAndKeepsSignalChannel);

} // namespace
