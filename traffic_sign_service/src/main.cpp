#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

#include "app/TrafficSignService.hpp"
#include "config/ServiceConfig.hpp"
#include "edge_impulse/EdgeImpulseClassifier.hpp"
#include "transport/VehicleWebSocketClient.hpp"

namespace {

volatile std::sig_atomic_t g_should_exit = 0;

void handleSignal(int) { g_should_exit = 1; }

} // namespace

int main(int argc, char **argv) {
    const std::string config_path = argc > 1 ? argv[1] : "config/service.env";

    traffic_sign_service::config::ServiceConfig config;
    std::vector<std::string> warnings;
    if (!traffic_sign_service::config::loadServiceConfigFromFile(config_path, config, &warnings)) {
        std::cerr << "Falha ao carregar service.env em " << config_path << std::endl;
        for (const auto &warning : warnings) {
            std::cerr << " - " << warning << std::endl;
        }
        return 1;
    }

    for (const auto &warning : warnings) {
        std::cout << "[config] " << warning << std::endl;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    auto transport =
        std::make_unique<traffic_sign_service::transport::VehicleWebSocketClient>(
            config.vehicle_ws_url);
    auto classifier =
        std::make_unique<traffic_sign_service::edge_impulse::EdgeImpulseClassifier>();

    traffic_sign_service::app::TrafficSignService service(
        config, std::move(transport), std::move(classifier));

    std::cout << "traffic_sign_service iniciado." << std::endl;
    std::cout << "Consumindo raw de " << config.vehicle_ws_url << std::endl;
    std::cout << "Modelo configurado em " << config.edge_impulse_model_zip_path << std::endl;
    std::cout << "Preview de frame: "
              << (config.frame_preview_enabled ? "habilitado" : "desabilitado")
              << std::endl;

    service.start();

    while (!g_should_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    service.stop();
    return 0;
}
