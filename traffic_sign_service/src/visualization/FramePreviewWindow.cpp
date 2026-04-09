#include "visualization/FramePreviewWindow.hpp"

#include <cstdlib>
#include <iostream>

#include <opencv2/highgui.hpp>

namespace {

bool hasDisplayEnvironment() {
    const char *display = std::getenv("DISPLAY");
    if (display && *display != '\0') {
        return true;
    }

    const char *wayland = std::getenv("WAYLAND_DISPLAY");
    return wayland && *wayland != '\0';
}

} // namespace

namespace traffic_sign_service::visualization {

FramePreviewWindow::FramePreviewWindow(bool enabled, std::string window_name)
    : enabled_{enabled}, window_name_{std::move(window_name)} {
    if (!enabled_) {
        return;
    }

    if (!hasDisplayEnvironment()) {
        enabled_ = false;
        std::cerr << "[preview] FRAME_PREVIEW_ENABLED=true, mas nenhum display grafico foi "
                     "detectado. Preview desabilitado."
                  << std::endl;
    }
}

FramePreviewWindow::~FramePreviewWindow() { close(); }

void FramePreviewWindow::show(const cv::Mat &frame) {
    if (!enabled_ || frame.empty()) {
        return;
    }

    try {
        if (!window_created_) {
            cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
            window_created_ = true;
            std::cout << "[preview] janela habilitada: " << window_name_ << std::endl;
        }

        cv::imshow(window_name_, frame);
        cv::waitKey(1);
    } catch (const cv::Exception &ex) {
        enabled_ = false;
        std::cerr << "[preview] falha ao exibir frame: " << ex.what()
                  << ". Preview desabilitado." << std::endl;
        close();
    }
}

void FramePreviewWindow::close() {
    if (!window_created_) {
        return;
    }

    try {
        cv::destroyWindow(window_name_);
        cv::waitKey(1);
    } catch (const cv::Exception &) {
    }

    window_created_ = false;
}

} // namespace traffic_sign_service::visualization
