#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "services/vision/VisionDebugStream.hpp"

namespace autonomous_car::services::autonomous_control {
class AutonomousControlService;
struct AutonomousControlSnapshot;
} // namespace autonomous_car::services::autonomous_control

namespace autonomous_car::services::traffic_sign_detection {
struct TrafficSignConfig;
class TrafficSignDetector;
} // namespace autonomous_car::services::traffic_sign_detection

namespace autonomous_car::services {

class RoadSegmentationService {
public:
    using TelemetryPublisher = std::function<void(const std::string &)>;
    using VisionFramePublisher =
        std::function<void(autonomous_car::services::vision::VisionDebugViewId,
                           const std::string &)>;
    using VisionSubscriptionProvider = std::function<
        autonomous_car::services::vision::VisionDebugViewSet()>;
    using ControlSink =
        std::function<void(const autonomous_car::services::autonomous_control::AutonomousControlSnapshot &)>;
    using TrafficSignDetectorFactory = std::function<std::unique_ptr<
        autonomous_car::services::traffic_sign_detection::TrafficSignDetector>(
        const autonomous_car::services::traffic_sign_detection::TrafficSignConfig &)>;

    RoadSegmentationService(
        std::string vision_config_path,
        autonomous_car::services::autonomous_control::AutonomousControlService *control_service = nullptr,
        TelemetryPublisher telemetry_publisher = {},
        VisionFramePublisher vision_frame_publisher = {},
        VisionSubscriptionProvider vision_subscription_provider = {},
        ControlSink control_sink = {},
        std::string window_name = "AutonomousCar Road Segmentation",
        TrafficSignDetectorFactory traffic_sign_detector_factory = {});
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
    TelemetryPublisher telemetry_publisher_;
    VisionFramePublisher vision_frame_publisher_;
    VisionSubscriptionProvider vision_subscription_provider_;
    ControlSink control_sink_;
    std::string window_name_;
    TrafficSignDetectorFactory traffic_sign_detector_factory_;
};

} // namespace autonomous_car::services
