#include "visualization/FramePreviewWindow.hpp"

#include <cmath>
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

bool FramePreviewWindow::show(const cv::Mat &frame) {
    if (!enabled_ || frame.empty()) {
        return true;
    }

    try {
        if (!window_created_) {
            cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
            window_created_ = true;
            std::cout << "[preview] janela habilitada: " << window_name_ << std::endl;
        }

        cv::imshow(window_name_, frame);
        frame_rendered_ = true;
        return poll(1);
    } catch (const cv::Exception &ex) {
        enabled_ = false;
        std::cerr << "[preview] falha ao exibir frame: " << ex.what()
                  << ". Preview desabilitado." << std::endl;
        close();
        return true;
    }
}

bool FramePreviewWindow::poll(int delay_ms) {
    if (!enabled_) {
        return true;
    }

    try {
        if (window_created_ && frame_rendered_ && visibility_check_supported_) {
            const double visible = cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE);
            if (std::isnan(visible) || visible < 0.0) {
                visibility_check_supported_ = false;
            } else if (visible < 1.0) {
                std::cout << "[preview] janela fechada pelo usuario." << std::endl;
                return false;
            }
        }
        const int key = cv::waitKey(delay_ms);
        return handleUiEvent(key);
    } catch (const cv::Exception &ex) {
        enabled_ = false;
        std::cerr << "[preview] backend grafico falhou durante waitKey/getWindowProperty: "
                  << ex.what() << ". Preview desabilitado." << std::endl;
        close();
        return true;
    }
}

bool FramePreviewWindow::handleUiEvent(int key) {
    switch (key) {
    case 27:
    case 'q':
    case 'Q':
        return false;
    default:
        return true;
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
    frame_rendered_ = false;
    visibility_check_supported_ = true;
}

} // namespace traffic_sign_service::visualization
