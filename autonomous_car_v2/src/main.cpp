#include "commands/CommandProcessor.h"
#include "config/VehicleConfig.h"
#include "managers/WebSocketManager.h"
#include "video/VideoStreamHandler.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <pigpio.h>
#include <thread>

std::atomic<bool> running(true);

void handleSignal(int) { running = false; }

int main() {
    std::signal(SIGINT, handleSignal);

    if (gpioInitialise() < 0) {
        std::cerr << "Falha ao iniciar pigpio" << std::endl;
        return 1;
    }

    CommandProcessor commandProcessor;

    WebSocketManager ws(8080);
    ws.setOnMessageCallback([&](const std::string &msg) {
        commandProcessor.processCommand(msg);
    });

    std::thread wsThreadObj([&] { ws.start(); });

    VideoStreamHandler video(4, [&](const std::string &frame) {
        ws.sendFrame(frame);
    });
    video.startStreaming();

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    video.stopStreaming();
    wsThreadObj.join();
    gpioTerminate();
    return 0;
}
