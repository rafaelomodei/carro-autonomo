#include "frame_source/LocalFrameSource.hpp"

#include <chrono>
#include <iostream>
#include <string_view>
#include <utility>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

namespace {

std::uint64_t currentTimestampMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string captureBackendName(const cv::VideoCapture &capture) {
    try {
        return capture.getBackendName();
    } catch (const cv::Exception &) {
        return "desconhecido";
    }
}

bool openCameraCapture(cv::VideoCapture &capture, int camera_index) {
    struct CameraBackendCandidate {
        int api;
        std::string_view label;
    };

    const CameraBackendCandidate candidates[] = {
        {cv::CAP_V4L2, "V4L2"},
        {cv::CAP_GSTREAMER, "GStreamer"},
        {cv::CAP_ANY, "CAP_ANY"},
    };

    for (const auto &candidate : candidates) {
        if (capture.open(camera_index, candidate.api)) {
            std::cout << "[source] camera aberta: index " << camera_index
                      << " | backend solicitado: " << candidate.label
                      << " | backend ativo: " << captureBackendName(capture) << std::endl;
            capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
            return true;
        }
    }

    return false;
}

} // namespace

namespace traffic_sign_service::frame_source {

ImageFrameSource::ImageFrameSource(std::string path) : path_{std::move(path)} {}

ImageFrameSource::~ImageFrameSource() { stop(); }

void ImageFrameSource::setFrameHandler(FrameHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    frame_handler_ = std::move(handler);
}

void ImageFrameSource::setEndHandler(EndHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    end_handler_ = std::move(handler);
}

void ImageFrameSource::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_thread_ = std::thread(&ImageFrameSource::run, this);
}

void ImageFrameSource::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

std::string ImageFrameSource::description() const { return "Imagem: " + path_; }

void ImageFrameSource::run() {
    const cv::Mat image = cv::imread(path_, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "[source] falha ao abrir imagem: " << path_ << std::endl;
    } else {
        FrameHandler handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = frame_handler_;
        }
        if (handler && running_.load()) {
            handler(FramePacket{image, currentTimestampMs()});
        }
    }

    EndHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = end_handler_;
    }
    if (handler) {
        handler();
    }
    running_ = false;
}

VideoFrameSource::VideoFrameSource(std::string path) : path_{std::move(path)} {}

VideoFrameSource::~VideoFrameSource() { stop(); }

void VideoFrameSource::setFrameHandler(FrameHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    frame_handler_ = std::move(handler);
}

void VideoFrameSource::setEndHandler(EndHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    end_handler_ = std::move(handler);
}

void VideoFrameSource::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_thread_ = std::thread(&VideoFrameSource::run, this);
}

void VideoFrameSource::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

std::string VideoFrameSource::description() const { return "Video: " + path_; }

void VideoFrameSource::run() {
    cv::VideoCapture capture;
    if (!capture.open(path_)) {
        std::cerr << "[source] falha ao abrir video: " << path_ << std::endl;
    } else {
        std::cout << "[source] video aberto: " << path_
                  << " | backend: " << captureBackendName(capture) << std::endl;
        const double fps = capture.get(cv::CAP_PROP_FPS);
        const auto delay = fps > 0.0 ? std::chrono::duration<double>(1.0 / fps)
                                     : std::chrono::milliseconds(33);
        std::size_t frames_emitted = 0;

        while (running_.load()) {
            cv::Mat frame;
            if (!capture.read(frame) || frame.empty()) {
                if (frames_emitted == 0) {
                    std::cerr << "[source] video abriu, mas nenhum frame foi lido: " << path_
                              << std::endl;
                } else {
                    std::cout << "[source] video encerrado apos " << frames_emitted
                              << " frame(s)." << std::endl;
                }
                break;
            }

            FrameHandler handler;
            {
                std::lock_guard<std::mutex> lock(handler_mutex_);
                handler = frame_handler_;
            }
            if (handler) {
                handler(FramePacket{frame, currentTimestampMs()});
            }
            ++frames_emitted;

            std::this_thread::sleep_for(
                std::chrono::duration_cast<std::chrono::milliseconds>(delay));
        }
    }

    EndHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = end_handler_;
    }
    if (handler) {
        handler();
    }
    running_ = false;
}

CameraFrameSource::CameraFrameSource(int camera_index) : camera_index_{camera_index} {}

CameraFrameSource::~CameraFrameSource() { stop(); }

void CameraFrameSource::setFrameHandler(FrameHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    frame_handler_ = std::move(handler);
}

void CameraFrameSource::setEndHandler(EndHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    end_handler_ = std::move(handler);
}

void CameraFrameSource::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_thread_ = std::thread(&CameraFrameSource::run, this);
}

void CameraFrameSource::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

std::string CameraFrameSource::description() const {
    return "Camera index " + std::to_string(camera_index_);
}

void CameraFrameSource::run() {
    cv::VideoCapture capture;
    if (!openCameraCapture(capture, camera_index_)) {
        std::cerr << "[source] falha ao abrir camera: " << camera_index_ << std::endl;
    } else {
        std::size_t frames_emitted = 0;
        while (running_.load()) {
            cv::Mat frame;
            if (!capture.read(frame) || frame.empty()) {
                if (frames_emitted == 0) {
                    std::cerr << "[source] camera abriu, mas nao entregou frames. Verifique o "
                                 "indice, permissoes em /dev/video* e se outro app esta usando "
                                 "a camera."
                              << std::endl;
                } else {
                    std::cout << "[source] camera interrompida apos " << frames_emitted
                              << " frame(s)." << std::endl;
                }
                break;
            }

            FrameHandler handler;
            {
                std::lock_guard<std::mutex> lock(handler_mutex_);
                handler = frame_handler_;
            }
            if (handler) {
                handler(FramePacket{frame, currentTimestampMs()});
            }
            ++frames_emitted;
        }
    }

    EndHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = end_handler_;
    }
    if (handler) {
        handler();
    }
    running_ = false;
}

} // namespace traffic_sign_service::frame_source
