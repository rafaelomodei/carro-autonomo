#pragma once

#include <functional>
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

} // namespace traffic_sign_service::tests
