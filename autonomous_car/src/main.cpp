#include "commands/CommandProcessor.h"
#include "managers/WebSocketManager.h"
#include "video/VideoStreamHandler.h"
#include "config/VehicleConfig.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <pigpio.h>            // se já não estava incluso em algum handler

// flag global para Ctrl-C
std::atomic<bool> stop(false);
void sigHandler(int) { stop = true; }

int main() {
    /* 1. Captura Ctrl-C */
    std::signal(SIGINT, sigHandler);

    /* 2. Inicializa pigpio uma única vez */
    if (gpioInitialise() < 0) {
        std::cerr << "Falha ao iniciar pigpio\n";
        return 1;
    }

    /* 3. Carrega configuração inicial */
    auto &vehicleConfig = VehicleConfig::getInstance();
    if (!vehicleConfig.loadFromFile("config/default_config.json")) {
        std::cerr << "Usando configurações padrão do veículo." << std::endl;
    }

    /* 4. Cria o processamento de comandos */
    CommandProcessor commandProcessor;

    /* 5. WebSocket em porta 8080 */
    WebSocketManager wsManager(8080);
    wsManager.setOnMessageCallback(
        [&commandProcessor](const std::string &msg) {
            commandProcessor.processCommand(msg);
        });

    /* 6. Câmera 4 -> callback envia quadro */
    VideoStreamHandler video(4, [&wsManager](const std::string &frame) {
        wsManager.sendFrame(frame);
    });

    /* 7. Sobe as threads */
    video.startStreaming();
    std::thread wsThread([&] { wsManager.start(); });   // io_context.run()

    /* 8. Loop de espera – Ctrl-C muda stop=false -> true */
    while (!stop) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    /* 9. Encerramento gracioso */
    video.stopStreaming();          // pára loop da câmera
    wsManager.stop();
    wsThread.join();
    gpioTerminate();                // fecha pigpio

    return 0;
}
