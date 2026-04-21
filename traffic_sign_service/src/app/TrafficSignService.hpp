#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <opencv2/core.hpp>

#include "concurrency/LatestFrameStore.hpp"
#include "config/ServiceConfig.hpp"
#include "domain/ITrafficSignClassifier.hpp"
#include "frame_source/IFrameSource.hpp"
#include "policy/DetectionPolicy.hpp"
#include "policy/TrafficSignTemporalFilter.hpp"
#include "transport/IMessagePublisher.hpp"
#include "visualization/FramePreviewWindow.hpp"
#include "visualization/TrafficSignDebugRenderer.hpp"

namespace traffic_sign_service::app {

class TrafficSignService {
public:
    using NowProvider = std::function<std::uint64_t()>;

    TrafficSignService(config::ServiceConfig config,
                       std::unique_ptr<frame_source::IFrameSource> frame_source,
                       std::unique_ptr<transport::IMessagePublisher> message_publisher,
                       std::unique_ptr<ITrafficSignClassifier> classifier,
                       NowProvider now_provider = {});
    ~TrafficSignService();

    void start();
    void stop();

    bool pollPreview(int delay_ms = 1);
    bool isCompleted() const noexcept;
    bool previewEnabled() const noexcept;

private:
    void handleSourceFrame(frame_source::FramePacket frame_packet);
    void handleSourceFinished();
    void inferenceLoop();
    void updateLatestDashboard(cv::Mat dashboard);
    TrafficSignRuntimeMetrics buildRuntimeMetrics(const TrafficSignFrameResult &result) const;

    config::ServiceConfig config_;
    std::unique_ptr<frame_source::IFrameSource> frame_source_;
    std::unique_ptr<transport::IMessagePublisher> message_publisher_;
    std::unique_ptr<ITrafficSignClassifier> classifier_;
    policy::TrafficSignTemporalFilter temporal_filter_;
    policy::DetectionPolicy detection_policy_;
    visualization::TrafficSignDebugRenderer debug_renderer_;
    visualization::FramePreviewWindow frame_preview_window_;
    concurrency::LatestFrameStore<frame_source::FramePacket> latest_frame_store_;
    NowProvider now_provider_;

    std::atomic<bool> running_{false};
    std::atomic<bool> source_finished_{false};
    std::atomic<bool> processing_completed_{false};
    std::atomic<std::uint64_t> received_frames_{0};
    std::atomic<std::uint64_t> processed_frames_{0};
    std::atomic<std::uint64_t> last_received_generation_{0};
    std::atomic<std::uint64_t> last_processed_generation_{0};
    std::atomic<double> service_fps_{0.0};
    std::atomic<double> inference_ms_{0.0};
    std::thread inference_thread_;

    mutable std::mutex latest_result_mutex_;
    std::optional<TrafficSignFrameResult> latest_result_;

    mutable std::mutex dashboard_mutex_;
    cv::Mat latest_dashboard_;
    std::uint64_t latest_dashboard_generation_{0};
    std::uint64_t displayed_dashboard_generation_{0};
};

} // namespace traffic_sign_service::app
