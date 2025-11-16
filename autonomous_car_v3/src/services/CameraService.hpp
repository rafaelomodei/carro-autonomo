#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace cv {
class VideoCapture;
}

namespace autonomous_car::services {

class CameraService {
  public:
    CameraService(int camera_index = 0, std::string window_name = "AutonomousCar Camera");
    ~CameraService();

    CameraService(const CameraService &) = delete;
    CameraService &operator=(const CameraService &) = delete;

    void start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

  private:
    void run();

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    int camera_index_;
    std::string window_name_;
};

} // namespace autonomous_car::services
