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
#include "commands/steering/SteeringCommand.hpp"
#include "commands/throttle/ThrottleCommand.hpp"
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
    using autonomous_car::commands::SteeringCommand;
    using autonomous_car::commands::StopCommand;
    using autonomous_car::commands::ThrottleCommand;
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
    steering_controller.configureAngleLimits(runtime_config.steering_center_angle,
                                             runtime_config.steering_left_limit,
                                             runtime_config.steering_right_limit);
    MotorController::DynamicsConfig motor_dynamics;
    motor_dynamics.invert_left = runtime_config.motor_left_inverted;
    motor_dynamics.invert_right = runtime_config.motor_right_inverted;
    motor_dynamics.command_timeout_ms = runtime_config.motor_command_timeout_ms;
    motor_controller.setDynamics(motor_dynamics);

    SteeringController::DynamicsConfig steering_dynamics;
    steering_dynamics.kp = runtime_config.steering_pid.kp;
    steering_dynamics.ki = runtime_config.steering_pid.ki;
    steering_dynamics.kd = runtime_config.steering_pid.kd;
    steering_dynamics.output_limit = runtime_config.steering_pid.output_limit;
    steering_dynamics.control_interval_ms = runtime_config.steering_pid.control_interval_ms;
    steering_controller.setDynamics(steering_dynamics);
    steering_controller.setSteeringSensitivity(runtime_config.steering_sensitivity);
    steering_controller.setCommandStep(runtime_config.steering_command_step);

    CommandDispatcher dispatcher;
    dispatcher.registerCommand("forward", std::make_unique<ForwardCommand>(motor_controller));
    dispatcher.registerCommand("backward", std::make_unique<BackwardCommand>(motor_controller));
    dispatcher.registerCommand("stop", std::make_unique<StopCommand>(motor_controller));
    dispatcher.registerCommand("throttle", std::make_unique<ThrottleCommand>(motor_controller));
    dispatcher.registerCommand("left", std::make_unique<TurnLeftCommand>(steering_controller));
    dispatcher.registerCommand("right", std::make_unique<TurnRightCommand>(steering_controller));
    dispatcher.registerCommand("center", std::make_unique<CenterSteeringCommand>(steering_controller));
    dispatcher.registerCommand("steering", std::make_unique<SteeringCommand>(steering_controller));

    auto config_update_handler = [&](const std::string &key, const std::string &value) {
        bool updated = config_manager.updateSetting(key, value);
        if (!updated) {
            return false;
        }
        auto updated_snapshot = config_manager.snapshot();
        MotorController::DynamicsConfig updated_motor_dynamics;
        updated_motor_dynamics.invert_left = updated_snapshot.motor_left_inverted;
        updated_motor_dynamics.invert_right = updated_snapshot.motor_right_inverted;
        updated_motor_dynamics.command_timeout_ms = updated_snapshot.motor_command_timeout_ms;
        motor_controller.setDynamics(updated_motor_dynamics);

        SteeringController::DynamicsConfig updated_steering_dynamics;
        updated_steering_dynamics.kp = updated_snapshot.steering_pid.kp;
        updated_steering_dynamics.ki = updated_snapshot.steering_pid.ki;
        updated_steering_dynamics.kd = updated_snapshot.steering_pid.kd;
        updated_steering_dynamics.output_limit = updated_snapshot.steering_pid.output_limit;
        updated_steering_dynamics.control_interval_ms = updated_snapshot.steering_pid.control_interval_ms;
        steering_controller.setDynamics(updated_steering_dynamics);
        steering_controller.setSteeringSensitivity(updated_snapshot.steering_sensitivity);
        steering_controller.setCommandStep(updated_snapshot.steering_command_step);
        steering_controller.configureAngleLimits(updated_snapshot.steering_center_angle,
                                                 updated_snapshot.steering_left_limit,
                                                 updated_snapshot.steering_right_limit);
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
