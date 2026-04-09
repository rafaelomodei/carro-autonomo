#include "vision/Base64.hpp"

#include <array>

namespace {

void setError(std::string *error, const std::string &message) {
    if (error) {
        *error = message;
    }
}

bool isBase64Char(unsigned char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
           ch == '+' || ch == '/';
}

} // namespace

namespace traffic_sign_service::vision {

bool isValidBase64(std::string_view value) {
    if (value.empty() || value.size() % 4 != 0) {
        return false;
    }

    std::size_t padding = 0;
    for (std::size_t index = value.size(); index > 0 && value[index - 1] == '='; --index) {
        ++padding;
    }

    if (padding > 2) {
        return false;
    }

    for (std::size_t i = 0; i < value.size() - padding; ++i) {
        if (!isBase64Char(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }

    for (std::size_t i = value.size() - padding; i < value.size(); ++i) {
        if (value[i] != '=') {
            return false;
        }
    }

    return true;
}

std::vector<unsigned char> decodeBase64(std::string_view value, std::string *error) {
    if (!isValidBase64(value)) {
        setError(error, "Payload base64 invalido.");
        return {};
    }

    static const std::array<int, 256> reverse_table = [] {
        std::array<int, 256> table{};
        table.fill(-1);
        for (int i = 'A'; i <= 'Z'; ++i) {
            table[static_cast<std::size_t>(i)] = i - 'A';
        }
        for (int i = 'a'; i <= 'z'; ++i) {
            table[static_cast<std::size_t>(i)] = i - 'a' + 26;
        }
        for (int i = '0'; i <= '9'; ++i) {
            table[static_cast<std::size_t>(i)] = i - '0' + 52;
        }
        table[static_cast<std::size_t>('+')] = 62;
        table[static_cast<std::size_t>('/')] = 63;
        return table;
    }();

    std::vector<unsigned char> decoded;
    decoded.reserve((value.size() / 4) * 3);

    for (std::size_t i = 0; i < value.size(); i += 4) {
        const int a = reverse_table[static_cast<std::size_t>(value[i])];
        const int b = reverse_table[static_cast<std::size_t>(value[i + 1])];
        const int c = value[i + 2] == '=' ? 0 : reverse_table[static_cast<std::size_t>(value[i + 2])];
        const int d = value[i + 3] == '=' ? 0 : reverse_table[static_cast<std::size_t>(value[i + 3])];

        const unsigned int triple =
            (static_cast<unsigned int>(a) << 18) | (static_cast<unsigned int>(b) << 12) |
            (static_cast<unsigned int>(c) << 6) | static_cast<unsigned int>(d);

        decoded.push_back(static_cast<unsigned char>((triple >> 16) & 0xFFu));
        if (value[i + 2] != '=') {
            decoded.push_back(static_cast<unsigned char>((triple >> 8) & 0xFFu));
        }
        if (value[i + 3] != '=') {
            decoded.push_back(static_cast<unsigned char>(triple & 0xFFu));
        }
    }

    return decoded;
}

std::string encodeBase64(const std::vector<unsigned char> &input) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    std::size_t index = 0;
    while (index < input.size()) {
        const std::size_t remaining = input.size() - index;
        const unsigned int octet_a = input[index++];
        const unsigned int octet_b = remaining > 1 ? input[index++] : 0;
        const unsigned int octet_c = remaining > 2 ? input[index++] : 0;

        const unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        encoded.push_back(table[(triple >> 18) & 0x3Fu]);
        encoded.push_back(table[(triple >> 12) & 0x3Fu]);
        encoded.push_back(remaining > 1 ? table[(triple >> 6) & 0x3Fu] : '=');
        encoded.push_back(remaining > 2 ? table[triple & 0x3Fu] : '=');
    }

    return encoded;
}

} // namespace traffic_sign_service::vision
