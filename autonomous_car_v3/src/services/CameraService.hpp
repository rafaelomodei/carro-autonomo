#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace cv {
class VideoCapture;
}

namespace autonomous_car::services {

namespace camera {
class LaneDetector;
class LaneVisualizer;
namespace algorithms {
class EdgeAnalyzer;
}
} // namespace camera

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
    void destroyWindows() const;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    int camera_index_;
    std::string window_name_;
    std::string processed_window_name_;
    std::string edges_window_name_;
    std::unique_ptr<camera::LaneDetector> lane_detector_;
    std::unique_ptr<camera::LaneVisualizer> lane_visualizer_;
    std::unique_ptr<camera::algorithms::EdgeAnalyzer> edge_analyzer_;
};

} // namespace autonomous_car::services
