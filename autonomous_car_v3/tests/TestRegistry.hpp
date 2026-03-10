#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace autonomous_car::tests {

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

inline void expectContains(std::string_view haystack, std::string_view needle,
                           const std::string &message) {
    if (haystack.find(needle) == std::string_view::npos) {
        throw std::runtime_error(message + " (needle=" + std::string(needle) + ")");
    }
}

} // namespace autonomous_car::tests
