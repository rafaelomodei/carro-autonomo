#include "commands/CommandProcessor.h"
#include "managers/WebSocketManager.h"
#include "video/VideoStreamHandler.h"
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

    /* 3. Cria o processamento de comandos */
    CommandProcessor commandProcessor;

    /* 4. WebSocket em porta 8080 */
    WebSocketManager wsManager(8080);
    wsManager.setOnMessageCallback(
        [&commandProcessor](const std::string &msg) {
            commandProcessor.processCommand(msg);
        });

    /* 5. Câmera 4 -> callback envia quadro */
    VideoStreamHandler video(4, [&wsManager](const std::string &frame) {
        wsManager.sendFrame(frame);
    });

    /* 6. Sobe as threads */
    video.startStreaming();
    std::thread wsThread([&] { wsManager.start(); });   // io_context.run()

    /* 7. Loop de espera – Ctrl-C muda stop=false -> true */
    while (!stop) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    /* 8. Encerramento gracioso */
    video.stopStreaming();          // pára loop da câmera
    // opcional: wsManager.stop();  // se criar método para dar ioContext.stop()
    wsThread.join();
    gpioTerminate();                // fecha pigpio

    return 0;
}
