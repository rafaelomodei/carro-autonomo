#include "services/CameraService.hpp"

#include <exception>
#include <iostream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "services/camera/LaneDetector.hpp"
#include "services/camera/LaneVisualizer.hpp"
#include "services/camera/algorithms/EdgeAnalyzer.hpp"

namespace autonomous_car::services {

CameraService::CameraService(int camera_index, std::string window_name)
    : camera_index_(camera_index),
      window_name_(std::move(window_name)),
      processed_window_name_(window_name_ + " - Processado"),
      edges_window_name_(window_name_ + " - Bordas"),
      lane_detector_(std::make_unique<camera::LaneDetector>()),
      lane_visualizer_(std::make_unique<camera::LaneVisualizer>()),
      edge_analyzer_(std::make_unique<camera::algorithms::EdgeAnalyzer>()) {}

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
        cv::namedWindow(processed_window_name_, cv::WINDOW_AUTOSIZE);
        cv::namedWindow(edges_window_name_, cv::WINDOW_AUTOSIZE);

        while (!stop_requested_.load()) {
            cv::Mat frame;
            if (!capture.read(frame)) {
                std::cerr << "[CameraService] Erro ao capturar imagem da câmera." << std::endl;
                break;
            }

            if (frame.empty()) {
                continue;
            }

            camera::LaneDetectionResult detection_result;
            if (lane_detector_) {
                detection_result = lane_detector_->detect(frame);
            }

            cv::Mat processed_view;
            if (lane_visualizer_) {
                processed_view = lane_visualizer_->buildDebugView(frame, detection_result);
            }

            cv::Mat edges_view;
            if (edge_analyzer_) {
                const auto edge_result = edge_analyzer_->analyze(frame);
                edges_view = edge_analyzer_->buildDebugView(frame, edge_result);
            }

            cv::imshow(window_name_, frame);
            if (!processed_view.empty()) {
                cv::imshow(processed_window_name_, processed_view);
            }
            if (!edges_view.empty()) {
                cv::imshow(edges_window_name_, edges_view);
            }
            // waitKey permite ao HighGUI atualizar a janela; 1ms evita bloquear o loop.
            if (cv::waitKey(1) == 27) { // tecla ESC
                std::cout << "[CameraService] Tecla ESC detectada. Encerrando captura." << std::endl;
                break;
            }
        }

        capture.release();
        destroyWindows();
    } catch (const std::exception &ex) {
        std::cerr << "[CameraService] Exceção ao executar serviço de câmera: " << ex.what()
                  << std::endl;
    }

    running_.store(false);
}

void CameraService::destroyWindows() const {
    if (!window_name_.empty()) {
        cv::destroyWindow(window_name_);
    }
    if (!processed_window_name_.empty()) {
        cv::destroyWindow(processed_window_name_);
    }
    if (!edges_window_name_.empty()) {
        cv::destroyWindow(edges_window_name_);
    }
}

} // namespace autonomous_car::services
