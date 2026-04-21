#pragma once

#include <string>

#include <opencv2/core/mat.hpp>

namespace traffic_sign_service::visualization {

class FramePreviewWindow {
public:
    explicit FramePreviewWindow(bool enabled,
                                std::string window_name = "traffic_sign_service.debug");
    ~FramePreviewWindow();

    bool show(const cv::Mat &frame);
    bool poll(int delay_ms = 1);
    bool enabled() const noexcept { return enabled_; }
    void close();

private:
    bool handleUiEvent(int key);

    bool enabled_{false};
    bool window_created_{false};
    bool frame_rendered_{false};
    bool visibility_check_supported_{true};
    std::string window_name_;
};

} // namespace traffic_sign_service::visualization
