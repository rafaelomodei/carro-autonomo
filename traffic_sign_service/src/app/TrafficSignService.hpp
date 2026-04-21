#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "concurrency/LatestFrameStore.hpp"
#include "config/ServiceConfig.hpp"
#include "domain/ITrafficSignClassifier.hpp"
#include "policy/DetectionPolicy.hpp"
#include "transport/IVehicleTransport.hpp"
#include "visualization/FramePreviewWindow.hpp"
#include "vision/VisionFrame.hpp"

namespace traffic_sign_service::app {

class TrafficSignService {
public:
    using NowProvider = std::function<std::uint64_t()>;

    TrafficSignService(config::ServiceConfig config,
                       std::unique_ptr<transport::IVehicleTransport> transport,
                       std::unique_ptr<ITrafficSignClassifier> classifier,
                       NowProvider now_provider = {});
    ~TrafficSignService();

    void start();
    void stop();

    void handleTransportOpened();
    void handleTransportMessage(const std::string &payload);

private:
    void inferenceLoop();
    std::optional<SignalDetection> selectPrimaryDetection(
        const std::vector<SignalDetection> &detections) const;

    config::ServiceConfig config_;
    std::unique_ptr<transport::IVehicleTransport> transport_;
    std::unique_ptr<ITrafficSignClassifier> classifier_;
    policy::DetectionPolicy detection_policy_;
    visualization::FramePreviewWindow frame_preview_window_;
    concurrency::LatestFrameStore<vision::VisionFrameMessage> latest_frame_store_;
    NowProvider now_provider_;
    std::atomic<bool> running_{false};
    std::thread inference_thread_;
};

} // namespace traffic_sign_service::app
