#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "common/DrivingMode.hpp"
#include "config/ConfigurationManager.hpp"
#include "controllers/CommandDispatcher.hpp"
#include "runtime/ConfigPathResolver.hpp"
#include "services/RoadSegmentationService.hpp"
#include "services/WebSocketServer.hpp"

namespace {
volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) {
    g_should_exit = 1;
}

} // namespace

int main() {
    using autonomous_car::config::ConfigurationManager;
    using autonomous_car::controllers::CommandDispatcher;
    using autonomous_car::runtime::resolveProjectPath;
    using autonomous_car::services::RoadSegmentationService;
    using autonomous_car::services::WebSocketServer;

    auto &config_manager = ConfigurationManager::instance();
    config_manager.loadDefaults();

    const auto config_path = resolveProjectPath("config/autonomous_car.env");
    config_manager.loadFromFile(config_path.string());
    std::cout << "Configuracao de hardware/logica: " << config_path << std::endl;

    const auto vision_config_path = resolveProjectPath("config/vision.env");
    std::cout << "Configuracao de visao: " << vision_config_path << std::endl;

    auto runtime_config = config_manager.snapshot();
    std::atomic<autonomous_car::DrivingMode> active_driving_mode{runtime_config.driving_mode};

    CommandDispatcher dispatcher;
    auto config_update_handler = [&](const std::string &key, const std::string &value) {
        const bool updated = config_manager.updateSetting(key, value);
        if (updated) {
            active_driving_mode.store(config_manager.snapshot().driving_mode);
        }
        return updated;
    };

    auto driving_mode_provider = [&active_driving_mode]() { return active_driving_mode.load(); };
    WebSocketServer server("0.0.0.0", 8080, dispatcher, config_update_handler, driving_mode_provider);
    RoadSegmentationService road_segmentation_service(
        vision_config_path.string(),
        [&server](const std::string &payload) { server.broadcastText(payload); });

    server.start();
    road_segmentation_service.start();

    std::cout << "autonomous_car_v3_vision_debug iniciado em ws://0.0.0.0:8080" << std::endl;
    std::cout << "Comandos de movimento nao estao ativos neste binario; apenas visao e telemetria."
              << std::endl;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    while (!g_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    road_segmentation_service.stop();
    server.stop();
    return 0;
}
