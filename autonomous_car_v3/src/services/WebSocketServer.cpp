#include "services/WebSocketServer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "controllers/CommandDispatcher.hpp"

namespace {
constexpr char kWebSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

class Sha1 {
public:
    Sha1() { reset(); }

    void update(const void *data, size_t len) {
        total_bytes_ += len;
        const auto *bytes = static_cast<const unsigned char *>(data);
        while (len--) {
            buffer_[buffer_size_++] = *bytes++;
            if (buffer_size_ == buffer_.size()) {
                processBlock(buffer_.data());
                buffer_size_ = 0;
            }
        }
    }

    std::array<unsigned char, 20> finalize() {
        uint64_t total_bits = total_bytes_ * 8;
        buffer_[buffer_size_++] = 0x80;
        if (buffer_size_ > 56) {
            while (buffer_size_ < 64) {
                buffer_[buffer_size_++] = 0x00;
            }
            processBlock(buffer_.data());
            buffer_size_ = 0;
        }
        while (buffer_size_ < 56) {
            buffer_[buffer_size_++] = 0x00;
        }
        for (int i = 7; i >= 0; --i) {
            buffer_[buffer_size_++] = static_cast<unsigned char>((total_bits >> (i * 8)) & 0xFF);
        }
        processBlock(buffer_.data());

        std::array<unsigned char, 20> digest{};
        for (size_t i = 0; i < 5; ++i) {
            digest[i * 4 + 0] = static_cast<unsigned char>((state_[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<unsigned char>((state_[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<unsigned char>((state_[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<unsigned char>(state_[i] & 0xFF);
        }
        reset();
        return digest;
    }

private:
    static uint32_t rotateLeft(uint32_t value, unsigned int bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    void processBlock(const unsigned char *block) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(block[i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];

        for (int i = 0; i < 80; ++i) {
            uint32_t f;
            uint32_t k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotateLeft(b, 30);
            b = a;
            a = temp;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
    }

    void reset() {
        state_[0] = 0x67452301;
        state_[1] = 0xEFCDAB89;
        state_[2] = 0x98BADCFE;
        state_[3] = 0x10325476;
        state_[4] = 0xC3D2E1F0;
        buffer_.fill(0);
        buffer_size_ = 0;
        total_bytes_ = 0;
    }

    std::array<uint32_t, 5> state_{};
    std::array<unsigned char, 64> buffer_{};
    size_t buffer_size_{0};
    uint64_t total_bytes_{0};
};

std::string sha1(const std::string &input) {
    Sha1 sha;
    sha.update(input.data(), input.size());
    auto digest = sha.finalize();
    return std::string(reinterpret_cast<const char *>(digest.data()), digest.size());
}

std::string base64Encode(const std::string &input) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    size_t index = 0;
    while (index < input.size()) {
        size_t remaining = input.size() - index;
        uint32_t octet_a = static_cast<unsigned char>(input[index++]);
        uint32_t octet_b = remaining > 1 ? static_cast<unsigned char>(input[index++]) : 0;
        uint32_t octet_c = remaining > 2 ? static_cast<unsigned char>(input[index++]) : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded.push_back(table[(triple >> 18) & 0x3F]);
        encoded.push_back(table[(triple >> 12) & 0x3F]);
        encoded.push_back(remaining > 1 ? table[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(remaining > 2 ? table[triple & 0x3F] : '=');
    }

    return encoded;
}

std::string computeAcceptKey(const std::string &client_key) {
    return base64Encode(sha1(client_key + kWebSocketGuid));
}

bool recvAll(int socket_fd, void *buffer, size_t length) {
    auto *bytes = static_cast<unsigned char *>(buffer);
    size_t received = 0;
    while (received < length) {
        ssize_t chunk = recv(socket_fd, bytes + received, length - received, 0);
        if (chunk <= 0) {
            return false;
        }
        received += static_cast<size_t>(chunk);
    }
    return true;
}

std::optional<std::string> readHttpRequest(int client_fd) {
    std::string request;
    std::array<char, 1024> buffer{};
    while (true) {
        ssize_t bytes = recv(client_fd, buffer.data(), buffer.size(), 0);
        if (bytes <= 0) {
            return std::nullopt;
        }
        request.append(buffer.data(), static_cast<size_t>(bytes));
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    return request;
}

std::optional<std::string> extractHeader(const std::string &request, const std::string &header_name) {
    std::istringstream stream(request);
    std::string line;
    std::string prefix = header_name + ":";
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.rfind(prefix, 0) == 0) {
            auto value = line.substr(prefix.size());
            auto begin = value.find_first_not_of(' ');
            if (begin != std::string::npos) {
                value = value.substr(begin);
            }
            return value;
        }
    }
    return std::nullopt;
}

bool sendHandshakeResponse(int client_fd, const std::string &accept_key) {
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << accept_key << "\r\n\r\n";
    auto response_str = response.str();
    ssize_t sent = send(client_fd, response_str.data(), response_str.size(), 0);
    return sent == static_cast<ssize_t>(response_str.size());
}

std::optional<std::string> readTextFrame(int client_fd) {
    unsigned char header[2];
    if (!recvAll(client_fd, header, sizeof(header))) {
        return std::nullopt;
    }

    bool fin = (header[0] & 0x80) != 0;
    unsigned char opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_length = header[1] & 0x7F;

    if (!fin) {
        return std::nullopt;
    }

    if (opcode == 0x8) {
        return std::nullopt;
    }

    if (!masked) {
        return std::nullopt;
    }

    if (payload_length == 126) {
        unsigned char extended[2];
        if (!recvAll(client_fd, extended, sizeof(extended))) {
            return std::nullopt;
        }
        payload_length = (static_cast<uint64_t>(extended[0]) << 8) | extended[1];
    } else if (payload_length == 127) {
        unsigned char extended[8];
        if (!recvAll(client_fd, extended, sizeof(extended))) {
            return std::nullopt;
        }
        payload_length = 0;
        for (int i = 0; i < 8; ++i) {
            payload_length = (payload_length << 8) | extended[i];
        }
    }

    unsigned char mask_key[4];
    if (!recvAll(client_fd, mask_key, sizeof(mask_key))) {
        return std::nullopt;
    }

    if (payload_length > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        return std::nullopt;
    }

    std::string payload(static_cast<size_t>(payload_length), '\0');
    if (!recvAll(client_fd, payload.data(), static_cast<size_t>(payload_length))) {
        return std::nullopt;
    }

    for (uint64_t i = 0; i < payload_length; ++i) {
        payload[static_cast<size_t>(i)] = payload[static_cast<size_t>(i)] ^ mask_key[i % 4];
    }

    if (opcode != 0x1) {
        return std::nullopt;
    }

    return payload;
}

void sendCloseFrame(int client_fd) {
    const unsigned char frame[2] = {0x88, 0x00};
    send(client_fd, frame, sizeof(frame), 0);
}

} // namespace

namespace autonomous_car::services {

WebSocketServer::WebSocketServer(const std::string &host, int port, controllers::CommandDispatcher &dispatcher)
    : host_{host},
      port_{port},
      dispatcher_{dispatcher},
      running_{false},
      server_fd_{-1} {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    server_thread_ = std::thread(&WebSocketServer::run, this);
}

void WebSocketServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    int fd = server_fd_.load();
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebSocketServer::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        running_ = false;
        return;
    }

    server_fd_.store(server_fd);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host_.c_str());
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        running_ = false;
        return;
    }

    if (listen(server_fd, 1) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd);
        running_ = false;
        return;
    }

    while (running_) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_address), &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }

        auto request_opt = readHttpRequest(client_fd);
        if (!request_opt) {
            close(client_fd);
            continue;
        }

        auto key_opt = extractHeader(*request_opt, "Sec-WebSocket-Key");
        if (!key_opt) {
            close(client_fd);
            continue;
        }

        auto accept_key = computeAcceptKey(*key_opt);
        if (!sendHandshakeResponse(client_fd, accept_key)) {
            close(client_fd);
            continue;
        }

        while (running_) {
            auto payload_opt = readTextFrame(client_fd);
            if (!payload_opt) {
                break;
            }

            auto command = *payload_opt;
            if (command == "ping") {
                continue;
            }
            bool handled = dispatcher_.dispatch(command);
            if (!handled) {
                std::cerr << "Unknown command received: " << command << std::endl;
            }
        }

        sendCloseFrame(client_fd);
        close(client_fd);
    }

    close(server_fd);
    server_fd_.store(-1);
}

} // namespace autonomous_car::services
