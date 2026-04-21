#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "app/TrafficSignService.hpp"
#include "config/ServiceConfig.hpp"
#include "edge_impulse/EdgeImpulseClassifier.hpp"
#include "edge_impulse/EimProcessClassifier.hpp"
#include "frame_source/LocalFrameSource.hpp"
#include "frame_source/WebSocketFrameSource.hpp"
#include "transport/IMessagePublisher.hpp"
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

    std::unique_ptr<traffic_sign_service::frame_source::IFrameSource> frame_source;
    std::unique_ptr<traffic_sign_service::transport::IMessagePublisher> message_publisher;

    if (config.frame_source_mode ==
        traffic_sign_service::FrameSourceMode::Image) {
        frame_source =
            std::make_unique<traffic_sign_service::frame_source::ImageFrameSource>(
                config.frame_source_path);
        message_publisher =
            std::make_unique<traffic_sign_service::transport::NullMessagePublisher>();
    } else if (config.frame_source_mode ==
               traffic_sign_service::FrameSourceMode::Video) {
        frame_source =
            std::make_unique<traffic_sign_service::frame_source::VideoFrameSource>(
                config.frame_source_path);
        message_publisher =
            std::make_unique<traffic_sign_service::transport::NullMessagePublisher>();
    } else if (config.frame_source_mode ==
               traffic_sign_service::FrameSourceMode::Camera) {
        frame_source =
            std::make_unique<traffic_sign_service::frame_source::CameraFrameSource>(
                config.camera_index);
        message_publisher =
            std::make_unique<traffic_sign_service::transport::NullMessagePublisher>();
    } else {
        auto transport =
            std::make_shared<traffic_sign_service::transport::VehicleWebSocketClient>(
                config.vehicle_ws_url);
        frame_source =
            std::make_unique<traffic_sign_service::frame_source::WebSocketFrameSource>(
                transport, "WebSocket: " + config.vehicle_ws_url);
        message_publisher =
            std::make_unique<traffic_sign_service::transport::TransportMessagePublisher>(
                transport);
    }

    std::unique_ptr<traffic_sign_service::ITrafficSignClassifier> classifier;
    if (config.edge_impulse_backend ==
        traffic_sign_service::EdgeImpulseBackend::Eim) {
        classifier =
            std::make_unique<traffic_sign_service::edge_impulse::EimProcessClassifier>(
                config);
    } else {
        classifier =
            std::make_unique<traffic_sign_service::edge_impulse::EdgeImpulseClassifier>(
                config);
    }

    traffic_sign_service::app::TrafficSignService service(
        config, std::move(frame_source), std::move(message_publisher),
        std::move(classifier));

    std::cout << "traffic_sign_service iniciado." << std::endl;
    std::cout << "Fonte: " << traffic_sign_service::toString(config.frame_source_mode)
              << std::endl;
    if ((config.frame_source_mode == traffic_sign_service::FrameSourceMode::Image ||
         config.frame_source_mode == traffic_sign_service::FrameSourceMode::Video) &&
        !config.frame_source_path.empty()) {
        std::cout << "Entrada local: " << config.frame_source_path << std::endl;
    } else if (config.frame_source_mode ==
               traffic_sign_service::FrameSourceMode::Camera) {
        std::cout << "Camera index: " << config.camera_index << std::endl;
    } else if (config.frame_source_mode ==
               traffic_sign_service::FrameSourceMode::WebSocket) {
        std::cout << "WebSocket: " << config.vehicle_ws_url << std::endl;
    }
    std::cout << "Backend: " << traffic_sign_service::toString(config.edge_impulse_backend)
              << std::endl;
    std::cout << "Preprocessamento grayscale: "
              << (config.traffic_sign_preprocess_grayscale ? "forcado" : "desligado")
              << std::endl;
    if (!config.edge_impulse_eim_path.empty()) {
        std::cout << "EIM: " << config.edge_impulse_eim_path << std::endl;
    }
    std::cout << "Preview de frame: "
              << (config.frame_preview_enabled ? "habilitado" : "desabilitado")
              << std::endl;
    if (config.edge_impulse_backend ==
        traffic_sign_service::EdgeImpulseBackend::EmbeddedCpp) {
        std::cout << "[perf] backend embedded_cpp usa o modelo compilado local e nao ativa GPU "
                     "neste caminho."
                  << std::endl;
    } else {
        std::cout << "[perf] backend eim usa o executavel Linux do Edge Impulse; no x86_64 o "
                     "fluxo oficial sobe em TFLite Full, enquanto o caminho GPU/TensorRT "
                     "documentado pelo Edge Impulse permanece focado em Jetson."
                  << std::endl;
    }

    service.start();
    bool completion_notified = false;

    while (!g_should_exit) {
        if (!service.pollPreview(30)) {
            break;
        }
        if (service.isCompleted() && service.previewEnabled() && !completion_notified) {
            std::cout << "[preview] processamento concluido; feche a janela ou pressione q para "
                         "encerrar."
                      << std::endl;
            completion_notified = true;
        }
        if (service.isCompleted() && !service.previewEnabled()) {
            break;
        }
    }

    service.stop();
    return 0;
}
