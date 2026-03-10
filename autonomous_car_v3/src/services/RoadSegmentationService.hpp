#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace autonomous_car::services {

class RoadSegmentationService {
public:
    using TelemetryPublisher = std::function<void(const std::string &)>;

    RoadSegmentationService(std::string vision_config_path, TelemetryPublisher publisher = {},
                            std::string window_name = "AutonomousCar Road Segmentation");
    ~RoadSegmentationService();

    RoadSegmentationService(const RoadSegmentationService &) = delete;
    RoadSegmentationService &operator=(const RoadSegmentationService &) = delete;

    void start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void run();
    void destroyWindow() const;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::string vision_config_path_;
    TelemetryPublisher publisher_;
    std::string window_name_;
};

} // namespace autonomous_car::services
