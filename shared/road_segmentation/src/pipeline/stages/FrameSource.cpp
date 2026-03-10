#include "pipeline/stages/FrameSource.hpp"

#include <stdexcept>

#include <opencv2/imgcodecs.hpp>

namespace road_segmentation_lab::pipeline::stages {

FrameSource::FrameSource(FrameSourceMode mode) : mode_(mode) {}

FrameSource FrameSource::fromInputPath(const std::string &path) {
    FrameSource source(FrameSourceMode::Image);
    source.image_ = cv::imread(path, cv::IMREAD_COLOR);
    if (!source.image_.empty()) {
        source.source_label_ = "Imagem: " + path;
        return source;
    }

    source.mode_ = FrameSourceMode::Video;
    if (!source.capture_.open(path)) {
        throw std::runtime_error("Nao foi possivel abrir a imagem ou video: " + path);
    }

    source.source_label_ = "Video: " + path;
    return source;
}

FrameSource FrameSource::fromCameraIndex(int camera_index) {
    FrameSource source(FrameSourceMode::Camera);
    if (!source.capture_.open(camera_index)) {
        throw std::runtime_error("Nao foi possivel abrir a camera de indice " +
                                 std::to_string(camera_index));
    }

    source.source_label_ = "Camera index " + std::to_string(camera_index);
    return source;
}

bool FrameSource::read(cv::Mat &frame) {
    if (mode_ == FrameSourceMode::Image) {
        if (image_.empty()) {
            return false;
        }
        frame = image_.clone();
        return true;
    }

    return capture_.read(frame);
}

bool FrameSource::isStaticImage() const noexcept { return mode_ == FrameSourceMode::Image; }

std::string FrameSource::description() const { return source_label_; }

} // namespace road_segmentation_lab::pipeline::stages
