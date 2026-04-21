#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace traffic_sign_service::tests {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase> &registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(std::string name, std::function<void()> fn) {
        registry().push_back({std::move(name), std::move(fn)});
    }
};

inline void expect(bool condition, const std::string &message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline void expectEqual(std::string_view lhs, std::string_view rhs, const std::string &message) {
    if (lhs != rhs) {
        throw std::runtime_error(message + " (lhs=" + std::string(lhs) + ", rhs=" +
                                 std::string(rhs) + ")");
    }
}

template <typename T, typename U>
inline void expectEqual(const T &lhs, const U &rhs, const std::string &message) {
    if (!(lhs == rhs)) {
        std::ostringstream stream;
        stream << message << " (lhs=" << lhs << ", rhs=" << rhs << ")";
        throw std::runtime_error(stream.str());
    }
}

inline void expectContains(std::string_view haystack, std::string_view needle,
                           const std::string &message) {
    if (haystack.find(needle) == std::string_view::npos) {
        throw std::runtime_error(message + " (needle=" + std::string(needle) + ")");
    }
}

} // namespace traffic_sign_service::tests
