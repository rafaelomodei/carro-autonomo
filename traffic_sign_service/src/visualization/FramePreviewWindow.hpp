#pragma once

#include <string>

#include <opencv2/core/mat.hpp>

namespace traffic_sign_service::visualization {

class FramePreviewWindow {
public:
    explicit FramePreviewWindow(bool enabled, std::string window_name = "traffic_sign_service.raw");
    ~FramePreviewWindow();

    void show(const cv::Mat &frame);
    void close();

private:
    bool enabled_{false};
    bool window_created_{false};
    std::string window_name_;
};

} // namespace traffic_sign_service::visualization
