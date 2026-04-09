#include <atomic>
#include <cstdint>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "common/DrivingMode.hpp"
#include "config/ConfigurationManager.hpp"
#include "controllers/CommandRouter.hpp"
#include "runtime/ConfigPathResolver.hpp"
#include "services/RoadSegmentationService.hpp"
#include "services/WebSocketServer.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"
#include "services/traffic_signals/TrafficSignalRegistry.hpp"

namespace {
volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) {
    g_should_exit = 1;
}

} // namespace

int main() {
    using autonomous_car::config::ConfigurationManager;
    using autonomous_car::controllers::CommandRouter;
    using autonomous_car::runtime::resolveProjectPath;
    using autonomous_car::services::RoadSegmentationService;
    using autonomous_car::services::WebSocketServer;
    using autonomous_car::services::traffic_signals::TrafficSignalRegistry;
    using autonomous_car::services::traffic_signals::toString;
    namespace autoctrl = autonomous_car::services::autonomous_control;

    auto &config_manager = ConfigurationManager::instance();
    config_manager.loadDefaults();

    const auto config_path = resolveProjectPath("config/autonomous_car.env");
    config_manager.loadFromFile(config_path.string());
    std::cout << "Configuracao de hardware/logica: " << config_path << std::endl;

    const auto vision_config_path = resolveProjectPath("config/vision.env");
    std::cout << "Configuracao de visao: " << vision_config_path << std::endl;

    auto runtime_config = config_manager.snapshot();
    std::atomic<autonomous_car::DrivingMode> active_driving_mode{runtime_config.driving_mode};
    autoctrl::AutonomousControlService autonomous_control_service;
    TrafficSignalRegistry traffic_signal_registry;
    autonomous_control_service.updateConfig(runtime_config.autonomous_control);
    autonomous_control_service.setDrivingMode(runtime_config.driving_mode);

    CommandRouter command_router;
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
        const bool updated = config_manager.updateSetting(key, value);
        if (updated) {
            const auto updated_snapshot = config_manager.snapshot();
            autonomous_control_service.updateConfig(updated_snapshot.autonomous_control);
            autonomous_control_service.setDrivingMode(updated_snapshot.driving_mode);
            active_driving_mode.store(updated_snapshot.driving_mode);
        }
        return updated;
    };

    auto driving_mode_provider = [&active_driving_mode]() { return active_driving_mode.load(); };
    auto signal_detected_handler = [&traffic_signal_registry](const std::string &signal_id) {
        const auto now_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
        const bool recorded = traffic_signal_registry.recordDetectedSignal(signal_id, now_ms);
        if (recorded) {
            const auto last_signal = traffic_signal_registry.lastDetectedSignal();
            if (last_signal) {
                std::cout << "Sinalizacao recebida em debug: "
                          << toString(last_signal->signal_id) << " em "
                          << last_signal->received_at_ms << " ms" << std::endl;
            }
        }
        return recorded;
    };
    WebSocketServer server("0.0.0.0", 8080, command_router, config_update_handler,
                           driving_mode_provider, signal_detected_handler);
    RoadSegmentationService road_segmentation_service(
        vision_config_path.string(),
        &autonomous_control_service,
        [&server](const std::string &payload) { server.broadcastText(payload); },
        [&server](auto view, const std::string &payload) {
            server.broadcastVisionFrame(view, payload);
        },
        [&server]() { return server.snapshotVisionSubscriptions(); });

    server.start();
    road_segmentation_service.start();

    std::cout << "autonomous_car_v3_vision_debug iniciado em ws://0.0.0.0:8080" << std::endl;
    std::cout << "Comandos manuais de movimento nao estao ativos neste binario; "
                 "autonomous:start/stop atualiza apenas o debug e a telemetria."
              << std::endl;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    while (!g_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    autonomous_control_service.stopAutonomous(autoctrl::StopReason::ServiceStop);
    road_segmentation_service.stop();
    server.stop();
    return 0;
}
