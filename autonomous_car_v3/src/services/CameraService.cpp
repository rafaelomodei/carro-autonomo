#include "services/CameraService.hpp"

#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "services/camera/LaneDetector.hpp"
#include "services/camera/LaneVisualizer.hpp"

namespace autonomous_car::services {

namespace {

std::string buildTimestampedFilename(const std::string &prefix, const std::string &extension) {
    const auto now = std::chrono::system_clock::now();
    const auto time_now = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm{};
    localtime_r(&time_now, &local_tm);

    std::ostringstream filename;
    filename << prefix << "_" << std::put_time(&local_tm, "%Y%m%d_%H%M%S") << extension;
    return filename.str();
}

bool openVideoWriter(cv::VideoWriter &writer, const std::filesystem::path &path, double fps,
                     const cv::Size &frame_size) {
    if (frame_size.width <= 0 || frame_size.height <= 0) {
        std::cerr << "[CameraService] Dimensoes invalidas para gravacao de video." << std::endl;
        return false;
    }

    struct CodecChoice {
        int fourcc;
        std::string name;
    };

    const CodecChoice choices[]{
        {cv::VideoWriter::fourcc('m', 'p', '4', 'v'), "mp4v"},
        {cv::VideoWriter::fourcc('a', 'v', 'c', '1'), "avc1"},
        {cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), "MJPG"},
    };

    for (const auto &choice : choices) {
        if (writer.open(path.string(), choice.fourcc, fps, frame_size, true)) {
            std::cout << "[CameraService] Gravando video com codec " << choice.name << " em: "
                      << path << std::endl;
            return true;
        }
        std::cerr << "[CameraService] Falha ao abrir video com codec " << choice.name
                  << " em: " << path << std::endl;
    }

    std::cerr << "[CameraService] Nao foi possivel abrir o arquivo de video apos todas as tentativas: "
              << path << std::endl;
    return false;
}

} // namespace

CameraService::CameraService(int camera_index, std::string window_name)
    : camera_index_(camera_index),
      window_name_(std::move(window_name)),
      processed_window_name_(window_name_ + " - Processado"),
      lane_detector_(std::make_unique<camera::LaneDetector>()),
      lane_visualizer_(std::make_unique<camera::LaneVisualizer>()) {}

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

        std::filesystem::path recordings_dir{"recordings"};
        std::error_code dir_error;
        std::filesystem::create_directories(recordings_dir, dir_error);
        if (dir_error) {
            std::cerr << "[CameraService] Aviso: nao foi possivel criar diretorio de gravacao: "
                      << recordings_dir << " - " << dir_error.message() << std::endl;
        }

        while (!stop_requested_.load()) {
            cv::Mat frame;
            if (!capture.read(frame)) {
                std::cerr << "[CameraService] Erro ao capturar imagem da câmera." << std::endl;
                break;
            }

            if (frame.empty()) {
                continue;
            }

            const cv::Size raw_size(frame.cols, frame.rows);
            double fps = capture.get(cv::CAP_PROP_FPS);
            if (fps <= 1.0) {
                fps = 30.0;
            }

            if (!raw_video_writer_.isOpened()) {
                auto raw_path = recordings_dir / buildTimestampedFilename("camera_raw", ".mp4");
                if (openVideoWriter(raw_video_writer_, raw_path, fps, raw_size)) {
                    std::cout << "[CameraService] Gravacao de video RAW iniciada: " << raw_path
                              << std::endl;
                }
            }

            camera::LaneDetectionResult detection_result;
            if (lane_detector_) {
                detection_result = lane_detector_->detect(frame);
            }

            cv::Mat processed_view;
            if (lane_visualizer_) {
                processed_view = lane_visualizer_->buildDebugView(frame, detection_result);
            }
            if (processed_view.empty()) {
                processed_view = frame.clone();
            }

            if (!processed_view.empty() && !processed_video_writer_.isOpened()) {
                auto processed_path =
                    recordings_dir / buildTimestampedFilename("camera_processado", ".mp4");
                const cv::Size processed_size(processed_view.cols, processed_view.rows);
                if (openVideoWriter(processed_video_writer_, processed_path, fps, processed_size)) {
                    std::cout << "[CameraService] Gravacao de video processado iniciada: "
                              << processed_path << std::endl;
                }
            }

            if (raw_video_writer_.isOpened()) {
                raw_video_writer_.write(frame);
            }

            if (processed_video_writer_.isOpened() && !processed_view.empty()) {
                processed_video_writer_.write(processed_view);
            }

            cv::imshow(window_name_, frame);
            if (!processed_view.empty()) {
                cv::imshow(processed_window_name_, processed_view);
            }
            // waitKey permite ao HighGUI atualizar a janela; 1ms evita bloquear o loop.
            if (cv::waitKey(1) == 27) { // tecla ESC
                std::cout << "[CameraService] Tecla ESC detectada. Encerrando captura." << std::endl;
                break;
            }
        }

        capture.release();
        if (raw_video_writer_.isOpened()) {
            raw_video_writer_.release();
        }
        if (processed_video_writer_.isOpened()) {
            processed_video_writer_.release();
        }
        destroyWindows();
    } catch (const std::exception &ex) {
        std::cerr << "[CameraService] Exceção ao executar serviço de câmera: " << ex.what()
                  << std::endl;
        if (raw_video_writer_.isOpened()) {
            raw_video_writer_.release();
        }
        if (processed_video_writer_.isOpened()) {
            processed_video_writer_.release();
        }
        destroyWindows();
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
}

} // namespace autonomous_car::services
