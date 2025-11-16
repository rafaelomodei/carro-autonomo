#include "services/CameraService.hpp"

#include <exception>
#include <iostream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

namespace autonomous_car::services {

CameraService::CameraService(int camera_index, std::string window_name)
    : camera_index_(camera_index), window_name_(std::move(window_name)) {}

CameraService::~CameraService() { stop(); }

void CameraService::start() {
    if (running_.load()) {
        return;
    }

    stop_requested_.store(false);
    worker_ = std::thread(&CameraService::run, this);
}

void CameraService::stop() {
    stop_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool CameraService::isRunning() const noexcept { return running_.load(); }

void CameraService::run() {
    try {
        cv::VideoCapture capture;
        if (!capture.open(camera_index_)) {
            std::cerr << "[CameraService] Aviso: não foi possível inicializar a câmera (índice "
                      << camera_index_ << ")." << std::endl;
            return;
        }

        running_.store(true);
        cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);

        while (!stop_requested_.load()) {
            cv::Mat frame;
            if (!capture.read(frame)) {
                std::cerr << "[CameraService] Erro ao capturar imagem da câmera." << std::endl;
                break;
            }

            if (frame.empty()) {
                continue;
            }

            cv::imshow(window_name_, frame);
            // waitKey permite ao HighGUI atualizar a janela; 1ms evita bloquear o loop.
            if (cv::waitKey(1) == 27) { // tecla ESC
                std::cout << "[CameraService] Tecla ESC detectada. Encerrando captura." << std::endl;
                break;
            }
        }

        capture.release();
        cv::destroyWindow(window_name_);
    } catch (const std::exception &ex) {
        std::cerr << "[CameraService] Exceção ao executar serviço de câmera: " << ex.what()
                  << std::endl;
    }

    running_.store(false);
}

} // namespace autonomous_car::services
