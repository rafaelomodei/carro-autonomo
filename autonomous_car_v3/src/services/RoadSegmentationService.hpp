#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace autonomous_car::services::autonomous_control {
class AutonomousControlService;
struct AutonomousControlSnapshot;
} // namespace autonomous_car::services::autonomous_control

namespace autonomous_car::services {

class RoadSegmentationService {
public:
    using TelemetryPublisher = std::function<void(const std::string &)>;
    using ControlSink =
        std::function<void(const autonomous_car::services::autonomous_control::AutonomousControlSnapshot &)>;

    RoadSegmentationService(
        std::string vision_config_path,
        autonomous_car::services::autonomous_control::AutonomousControlService *control_service = nullptr,
        TelemetryPublisher publisher = {}, ControlSink control_sink = {},
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
    autonomous_car::services::autonomous_control::AutonomousControlService *control_service_;
    TelemetryPublisher publisher_;
    ControlSink control_sink_;
    std::string window_name_;
};

} // namespace autonomous_car::services
