#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include <softPwm.h>
#include <wiringPi.h>

#include "commands/BackwardCommand.hpp"
#include "commands/CenterSteeringCommand.hpp"
#include "commands/ForwardCommand.hpp"
#include "commands/StopCommand.hpp"
#include "commands/TurnLeftCommand.hpp"
#include "commands/TurnRightCommand.hpp"
#include "controllers/CommandDispatcher.hpp"
#include "controllers/MotorController.hpp"
#include "controllers/SteeringController.hpp"
#include "services/WebSocketServer.hpp"

namespace {
volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) {
    g_should_exit = 1;
}

constexpr int kMotorPinForwardA = 17;
constexpr int kMotorPinBackwardA = 27;
constexpr int kMotorPinForwardB = 23;
constexpr int kMotorPinBackwardB = 22;
constexpr int kSteeringPwmPin = 18;
}

int main() {
    if (wiringPiSetupGpio() != 0) {
        std::cerr << "Falha ao inicializar o WiringPi. Certifique-se de executá-lo com permissões adequadas." << std::endl;
        return 1;
    }

    controllers::MotorController motor_controller(kMotorPinForwardA, kMotorPinBackwardA,
                                                  kMotorPinForwardB, kMotorPinBackwardB);
    controllers::SteeringController steering_controller(kSteeringPwmPin);

    controllers::CommandDispatcher dispatcher;
    dispatcher.registerCommand("forward", std::make_unique<commands::ForwardCommand>(motor_controller));
    dispatcher.registerCommand("backward", std::make_unique<commands::BackwardCommand>(motor_controller));
    dispatcher.registerCommand("stop", std::make_unique<commands::StopCommand>(motor_controller));
    dispatcher.registerCommand("left", std::make_unique<commands::TurnLeftCommand>(steering_controller));
    dispatcher.registerCommand("right", std::make_unique<commands::TurnRightCommand>(steering_controller));
    dispatcher.registerCommand("center", std::make_unique<commands::CenterSteeringCommand>(steering_controller));

    services::WebSocketServer server("0.0.0.0", 8080, dispatcher);
    server.start();

    std::cout << "Autonomous Car v3 WebSocket server iniciado em ws://0.0.0.0:8080" << std::endl;
    std::cout << "Use comandos: forward, backward, stop, left, right, center" << std::endl;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    while (!g_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Encerrando servidor..." << std::endl;
    server.stop();
    motor_controller.stop();
    steering_controller.center();

    return 0;
}
