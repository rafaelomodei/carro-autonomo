#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "frame_source/IFrameSource.hpp"

namespace traffic_sign_service::frame_source {

class ImageFrameSource final : public IFrameSource {
public:
    explicit ImageFrameSource(std::string path);
    ~ImageFrameSource() override;

    void setFrameHandler(FrameHandler handler) override;
    void setEndHandler(EndHandler handler) override;
    void start() override;
    void stop() override;
    bool isFinite() const noexcept override { return true; }
    std::string description() const override;

private:
    void run();

    std::string path_;
    FrameHandler frame_handler_;
    EndHandler end_handler_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex handler_mutex_;
};

class VideoFrameSource final : public IFrameSource {
public:
    explicit VideoFrameSource(std::string path);
    ~VideoFrameSource() override;

    void setFrameHandler(FrameHandler handler) override;
    void setEndHandler(EndHandler handler) override;
    void start() override;
    void stop() override;
    bool isFinite() const noexcept override { return true; }
    std::string description() const override;

private:
    void run();

    std::string path_;
    FrameHandler frame_handler_;
    EndHandler end_handler_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex handler_mutex_;
};

class CameraFrameSource final : public IFrameSource {
public:
    explicit CameraFrameSource(int camera_index);
    ~CameraFrameSource() override;

    void setFrameHandler(FrameHandler handler) override;
    void setEndHandler(EndHandler handler) override;
    void start() override;
    void stop() override;
    bool isFinite() const noexcept override { return false; }
    std::string description() const override;

private:
    void run();

    int camera_index_{0};
    FrameHandler frame_handler_;
    EndHandler end_handler_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex handler_mutex_;
};

} // namespace traffic_sign_service::frame_source
