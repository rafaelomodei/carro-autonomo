#include "managers/WebSocketManager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

std::atomic<bool> running(true);

void handleSignal(int) { running = false; }

int main() {
    std::signal(SIGINT, handleSignal);

    WebSocketManager ws(8080);
    ws.setOnMessageCallback([](const std::string &msg) {
        std::cout << "Received: " << msg << std::endl;
    });

    std::thread wsThread([&] { ws.start(); });

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    wsThread.join();
    return 0;
}
