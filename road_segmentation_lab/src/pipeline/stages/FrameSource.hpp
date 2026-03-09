#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <string>

namespace road_segmentation_lab::pipeline::stages {

enum class FrameSourceMode {
    Image,
    Video,
    Camera,
};

class FrameSource {
  public:
    static FrameSource fromInputPath(const std::string &path);
    static FrameSource fromCameraIndex(int camera_index);

    bool read(cv::Mat &frame);
    bool isStaticImage() const noexcept;
    std::string description() const;

  private:
    explicit FrameSource(FrameSourceMode mode);

    FrameSourceMode mode_;
    std::string source_label_;
    cv::Mat image_;
    cv::VideoCapture capture_;
};

} // namespace road_segmentation_lab::pipeline::stages
