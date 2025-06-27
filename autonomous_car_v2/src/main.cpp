#include "managers/WebSocketManager.h"
#include "config/VehicleConfig.h"
#include "video/VideoStreamHandler.h"
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

    VehicleConfig &config = VehicleConfig::getInstance();
    (void)config; // apenas para evitar aviso se não usado

    std::thread wsThread([&] { ws.start(); });

    VideoStreamHandler video(0, [&](const std::string &frame) {
        ws.sendFrame(frame);
    });
    video.startStreaming();

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    video.stopStreaming();
    wsThread.join();
    return 0;
}
