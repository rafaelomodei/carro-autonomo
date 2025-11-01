#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <memory>
#include <thread>

#include <wiringPi.h>

#include "commands/backward/BackwardCommand.hpp"
#include "commands/center_steering/CenterSteeringCommand.hpp"
#include "commands/forward/ForwardCommand.hpp"
#include "commands/stop/StopCommand.hpp"
#include "commands/turn_left/TurnLeftCommand.hpp"
#include "commands/turn_right/TurnRightCommand.hpp"
#include "config/ConfigurationManager.hpp"
#include "controllers/CommandDispatcher.hpp"
#include "controllers/MotorController.hpp"
#include "controllers/SteeringController.hpp"
#include "services/WebSocketServer.hpp"

namespace {
volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) {
    g_should_exit = 1;
}

std::filesystem::path resolveConfigPath() {
    namespace fs = std::filesystem;
    const fs::path config_relative{"config/autonomous_car.env"};

    std::error_code error;
    auto executable_symlink = fs::read_symlink("/proc/self/exe", error);
    if (!error) {
        auto candidate = executable_symlink.parent_path().parent_path() / config_relative;
        if (fs::exists(candidate)) {
            return candidate;
        }
    }

    fs::path fallback = fs::current_path() / config_relative;
    if (fs::exists(fallback)) {
        return fallback;
    }

    return {};
}
}

int main() {
    if (wiringPiSetupGpio() != 0) {
        std::cerr << "Falha ao inicializar o WiringPi. Certifique-se de executá-lo com permissões adequadas." << std::endl;
        return 1;
    }

    using autonomous_car::config::ConfigurationManager;
    using autonomous_car::commands::BackwardCommand;
    using autonomous_car::commands::CenterSteeringCommand;
    using autonomous_car::commands::ForwardCommand;
    using autonomous_car::commands::StopCommand;
    using autonomous_car::commands::TurnLeftCommand;
    using autonomous_car::commands::TurnRightCommand;
    using autonomous_car::controllers::CommandDispatcher;
    using autonomous_car::controllers::MotorController;
    using autonomous_car::controllers::SteeringController;
    using autonomous_car::services::WebSocketServer;

    auto &config_manager = ConfigurationManager::instance();
    config_manager.loadDefaults();
    auto config_path = resolveConfigPath();
    if (!config_path.empty()) {
        config_manager.loadFromFile(config_path.string());
        std::cout << "Configuração carregada de: " << config_path << std::endl;
    } else {
        std::cerr << "Arquivo de configuração não encontrado. Utilizando valores padrão." << std::endl;
    }

    auto runtime_config = config_manager.snapshot();

    MotorController motor_controller(runtime_config.motor_pins.forward_left,
                                     runtime_config.motor_pins.backward_left,
                                     runtime_config.motor_pins.forward_right,
                                     runtime_config.motor_pins.backward_right);
    SteeringController steering_controller(runtime_config.steering_pwm_pin);
    steering_controller.setSteeringSensitivity(runtime_config.steering_sensitivity);

    CommandDispatcher dispatcher;
    dispatcher.registerCommand("forward", std::make_unique<ForwardCommand>(motor_controller));
    dispatcher.registerCommand("backward", std::make_unique<BackwardCommand>(motor_controller));
    dispatcher.registerCommand("stop", std::make_unique<StopCommand>(motor_controller));
    dispatcher.registerCommand("left", std::make_unique<TurnLeftCommand>(steering_controller));
    dispatcher.registerCommand("right", std::make_unique<TurnRightCommand>(steering_controller));
    dispatcher.registerCommand("center", std::make_unique<CenterSteeringCommand>(steering_controller));

    auto config_update_handler = [&](const std::string &key, const std::string &value) {
        bool updated = config_manager.updateSetting(key, value);
        if (!updated) {
            return false;
        }
        auto updated_snapshot = config_manager.snapshot();
        steering_controller.setSteeringSensitivity(updated_snapshot.steering_sensitivity);
        return true;
    };

    WebSocketServer server("0.0.0.0", 8080, dispatcher, config_update_handler);
    server.start();

    std::cout << "Autonomous Car v3 WebSocket server iniciado em ws://0.0.0.0:8080" << std::endl;
    std::cout << "Canal de comandos: command:<acao> (ex.: command:forward)" << std::endl;
    std::cout << "Canal de configuração: config:<chave>=<valor> (ex.: config:steering.sensitivity=1.2)" << std::endl;

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
