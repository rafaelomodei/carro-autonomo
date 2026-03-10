#include <exception>
#include <iostream>

#include "TestRegistry.hpp"

int main() {
    using autonomous_car::tests::registry;

    int failed = 0;
    for (const auto &test : registry()) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << std::endl;
        } catch (const std::exception &ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << std::endl;
        }
    }

    if (failed > 0) {
        std::cerr << failed << " teste(s) falharam." << std::endl;
        return 1;
    }

    std::cout << registry().size() << " teste(s) executados com sucesso." << std::endl;
    return 0;
}
