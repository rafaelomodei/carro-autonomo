#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include <wiringPi.h>

#include "commands/backward/BackwardCommand.hpp"
#include "commands/center_steering/CenterSteeringCommand.hpp"
#include "commands/forward/ForwardCommand.hpp"
#include "commands/steering/SteeringCommand.hpp"
#include "commands/stop/StopCommand.hpp"
#include "commands/throttle/ThrottleCommand.hpp"
#include "commands/turn_left/TurnLeftCommand.hpp"
#include "commands/turn_right/TurnRightCommand.hpp"
#include "common/DrivingMode.hpp"
#include "config/ConfigurationManager.hpp"
#include "controllers/CommandDispatcher.hpp"
#include "controllers/CommandRouter.hpp"
#include "controllers/MotorController.hpp"
#include "controllers/SteeringController.hpp"
#include "runtime/ConfigPathResolver.hpp"
#include "services/RoadSegmentationService.hpp"
#include "services/WebSocketServer.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"

namespace {
volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) {
    g_should_exit = 1;
}

} // namespace

int main() {
    if (wiringPiSetupGpio() != 0) {
        std::cerr << "Falha ao inicializar o WiringPi. Certifique-se de executa-lo com permissoes adequadas."
                  << std::endl;
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
    using autonomous_car::controllers::CommandRouter;
    using autonomous_car::controllers::MotorController;
    using autonomous_car::controllers::SteeringController;
    using autonomous_car::runtime::resolveProjectPath;
    using autonomous_car::services::RoadSegmentationService;
    using autonomous_car::services::WebSocketServer;
    namespace autoctrl = autonomous_car::services::autonomous_control;

    auto &config_manager = ConfigurationManager::instance();
    config_manager.loadDefaults();

    const auto config_path = resolveProjectPath("config/autonomous_car.env");
    config_manager.loadFromFile(config_path.string());
    std::cout << "Configuracao de hardware: " << config_path << std::endl;

    const auto vision_config_path = resolveProjectPath("config/vision.env");
    std::cout << "Configuracao de visao: " << vision_config_path << std::endl;

    auto runtime_config = config_manager.snapshot();
    std::atomic<autonomous_car::DrivingMode> active_driving_mode{runtime_config.driving_mode};
    autoctrl::AutonomousControlService autonomous_control_service;
    autonomous_control_service.updateConfig(runtime_config.autonomous_control);
    autonomous_control_service.setDrivingMode(runtime_config.driving_mode);

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

    steering_controller.setSteeringSensitivity(runtime_config.steering_sensitivity);
    steering_controller.setCommandStep(runtime_config.steering_command_step);

    CommandDispatcher dispatcher;
    CommandRouter command_router;
    dispatcher.registerCommand("forward", std::make_unique<ForwardCommand>(motor_controller));
    dispatcher.registerCommand("backward", std::make_unique<BackwardCommand>(motor_controller));
    dispatcher.registerCommand("stop", std::make_unique<StopCommand>(motor_controller));
    dispatcher.registerCommand("throttle", std::make_unique<ThrottleCommand>(motor_controller));
    dispatcher.registerCommand("left", std::make_unique<TurnLeftCommand>(steering_controller));
    dispatcher.registerCommand("right", std::make_unique<TurnRightCommand>(steering_controller));
    dispatcher.registerCommand("center", std::make_unique<CenterSteeringCommand>(steering_controller));
    dispatcher.registerCommand("steering", std::make_unique<SteeringCommand>(steering_controller));

    const auto register_manual_command = [&](const std::string &name) {
        command_router.registerCommand(
            name, autonomous_car::CommandSource::Manual,
            [&dispatcher, name](std::optional<double> value) { dispatcher.dispatch(name, value); });
    };

    register_manual_command("forward");
    register_manual_command("backward");
    register_manual_command("stop");
    register_manual_command("throttle");
    register_manual_command("left");
    register_manual_command("right");
    register_manual_command("center");
    register_manual_command("steering");

    command_router.registerCommand("start", autonomous_car::CommandSource::Autonomous,
                                   [&autonomous_control_service](std::optional<double>) {
                                       autonomous_control_service.startAutonomous();
                                   });
    command_router.registerCommand("stop", autonomous_car::CommandSource::Autonomous,
                                   [&autonomous_control_service](std::optional<double>) {
                                       autonomous_control_service.stopAutonomous(
                                           autoctrl::StopReason::CommandStop);
                                   });

    auto config_update_handler = [&](const std::string &key, const std::string &value) {
        const auto previous_snapshot = config_manager.snapshot();
        const bool updated = config_manager.updateSetting(key, value);
        if (!updated) {
            return false;
        }

        const auto updated_snapshot = config_manager.snapshot();
        MotorController::DynamicsConfig updated_motor_dynamics;
        updated_motor_dynamics.invert_left = updated_snapshot.motor_left_inverted;
        updated_motor_dynamics.invert_right = updated_snapshot.motor_right_inverted;
        updated_motor_dynamics.command_timeout_ms = updated_snapshot.motor_command_timeout_ms;
        motor_controller.setDynamics(updated_motor_dynamics);

        steering_controller.setSteeringSensitivity(updated_snapshot.steering_sensitivity);
        steering_controller.setCommandStep(updated_snapshot.steering_command_step);
        steering_controller.configureAngleLimits(updated_snapshot.steering_center_angle,
                                                 updated_snapshot.steering_left_limit,
                                                 updated_snapshot.steering_right_limit);
        autonomous_control_service.updateConfig(updated_snapshot.autonomous_control);
        autonomous_control_service.setDrivingMode(updated_snapshot.driving_mode);

        if (updated_snapshot.driving_mode != previous_snapshot.driving_mode) {
            motor_controller.stop();
            steering_controller.center();
        }

        active_driving_mode.store(updated_snapshot.driving_mode);
        return true;
    };

    auto driving_mode_provider = [&active_driving_mode]() { return active_driving_mode.load(); };
    WebSocketServer server("0.0.0.0", 8080, command_router, config_update_handler,
                           driving_mode_provider);
    auto autonomous_sink =
        [&motor_controller, &steering_controller](
            const autoctrl::AutonomousControlSnapshot &snapshot) {
            if (snapshot.driving_mode != autonomous_car::DrivingMode::Autonomous) {
                return;
            }

            steering_controller.setSteering(snapshot.steering_command);
            if (snapshot.motion_command == autoctrl::MotionCommand::Forward) {
                motor_controller.forward(1.0);
            } else {
                motor_controller.stop();
                steering_controller.center();
            }
        };
    RoadSegmentationService road_segmentation_service(
        vision_config_path.string(),
        &autonomous_control_service,
        [&server](const std::string &payload) { server.broadcastText(payload); },
        [&server](auto view, const std::string &payload) {
            server.broadcastVisionFrame(view, payload);
        },
        [&server]() { return server.snapshotVisionSubscriptions(); },
        autonomous_sink);

    server.start();
    road_segmentation_service.start();

    std::cout << "Autonomous Car v3 iniciado em ws://0.0.0.0:8080" << std::endl;
    std::cout << "Canais: command:<origem>:<acao>, config:<chave>=<valor>, "
                 "client:control/client:telemetry"
              << std::endl;
    std::cout << "Modo autonomo: command:autonomous:start | command:autonomous:stop"
              << std::endl;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    while (!g_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Encerrando servidor..." << std::endl;
    autonomous_control_service.stopAutonomous(autoctrl::StopReason::ServiceStop);
    road_segmentation_service.stop();
    server.stop();
    motor_controller.stop();
    steering_controller.center();

    return 0;
}
