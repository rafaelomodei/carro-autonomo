#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "TestRegistry.hpp"
#include "services/RoadSegmentationService.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::tests::expectContains;
namespace autoctrl = autonomous_car::services::autonomous_control;
namespace services = autonomous_car::services;
namespace vision = autonomous_car::services::vision;

struct CapturedServiceOutput {
    std::vector<std::string> telemetry;
    std::vector<std::pair<vision::VisionDebugViewId, std::string>> frames;
    mutable std::mutex mutex;
};

std::filesystem::path writeTextFile(const std::string &name, const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

std::filesystem::path createStaticTestImage(const std::string &name) {
    cv::Mat frame(240, 320, CV_8UC3, cv::Scalar{25, 25, 25});
    cv::line(frame, {120, 239}, {150, 120}, {255, 255, 255}, 8, cv::LINE_AA);
    cv::line(frame, {200, 239}, {170, 120}, {255, 255, 255}, 8, cv::LINE_AA);

    const auto image_path = std::filesystem::temp_directory_path() / name;
    cv::imwrite(image_path.string(), frame);
    return image_path;
}

std::filesystem::path createVideoTestFile(const std::string &name, int frame_count) {
    const auto video_path = std::filesystem::temp_directory_path() / name;
    cv::VideoWriter writer(
        video_path.string(), cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 15.0, {320, 240});
    if (!writer.isOpened()) {
        throw std::runtime_error("Nao foi possivel criar o video temporario de teste.");
    }

    for (int index = 0; index < frame_count; ++index) {
        cv::Mat frame(240, 320, CV_8UC3, cv::Scalar{25, 25, 25});
        cv::line(frame, {118 + (index % 6), 239}, {148, 120}, {255, 255, 255}, 8, cv::LINE_AA);
        cv::line(frame, {202 - (index % 6), 239}, {172, 120}, {255, 255, 255}, 8, cv::LINE_AA);
        writer.write(frame);
    }

    writer.release();
    return video_path;
}

std::filesystem::path createVisionConfig(const std::string &source_mode,
                                         const std::filesystem::path &source_path,
                                         double telemetry_max_fps = 30.0,
                                         double stream_max_fps = 30.0) {
    const auto source_root =
        std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path();
    const auto segmentation_path = source_root / "config/road_segmentation.env";

    return writeTextFile(
        "road_segmentation_service_vision.env",
        "VISION_SOURCE_MODE=" + source_mode + "\n"
        "VISION_SOURCE_PATH=" + source_path.string() + "\n"
        "VISION_DEBUG_WINDOW_ENABLED=false\n"
        "VISION_TELEMETRY_MAX_FPS=" + std::to_string(telemetry_max_fps) + "\n"
        "VISION_STREAM_MAX_FPS=" + std::to_string(stream_max_fps) + "\n"
        "VISION_STREAM_JPEG_QUALITY=70\n"
        "VISION_SEGMENTATION_CONFIG_PATH=" + segmentation_path.string() + "\n");
}

bool waitForCondition(const std::function<bool()> &predicate, int timeout_ms = 3000) {
    const auto start = std::chrono::steady_clock::now();
    while (!predicate()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count() > timeout_ms) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return true;
}

std::string findTelemetryByType(const CapturedServiceOutput &output, const std::string &type) {
    std::lock_guard<std::mutex> lock(output.mutex);
    std::string last_match;
    for (const auto &payload : output.telemetry) {
        if (payload.find(type) != std::string::npos) {
            last_match = payload;
        }
    }
    return last_match;
}

std::size_t frameCount(const CapturedServiceOutput &output) {
    std::lock_guard<std::mutex> lock(output.mutex);
    return output.frames.size();
}

void testServicePublishesCoreTelemetryAndStreamWithoutLocalTrafficSignPipeline() {
    const auto image_path = createStaticTestImage("road_segmentation_service_static.png");
    const auto vision_config_path = createVisionConfig("image", image_path);

    autoctrl::AutonomousControlService control_service;
    CapturedServiceOutput output;
    services::RoadSegmentationService service(
        vision_config_path.string(), &control_service,
        [&](const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.telemetry.push_back(payload);
        },
        [&](vision::VisionDebugViewId view, const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.frames.emplace_back(view, payload);
        },
        [] {
            return vision::VisionDebugViewSet{vision::VisionDebugViewId::Raw,
                                              vision::VisionDebugViewId::Dashboard};
        });

    service.start();
    const bool ready = waitForCondition(
        [&] {
            return !findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"")
                        .empty() &&
                   !findTelemetryByType(output, "\"type\":\"telemetry.autonomous_control\"")
                        .empty() &&
                   !findTelemetryByType(output, "\"type\":\"telemetry.vision_runtime\"")
                        .empty() &&
                   frameCount(output) >= 2;
        },
        4000);
    service.stop();

    expect(ready, "Servico deve publicar stream e telemetrias principais.");

    const std::string runtime_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.vision_runtime\"");
    expectContains(runtime_payload, "\"traffic_sign_fps\":0.000000",
                   "Runtime deve zerar FPS de sinalizacao local.");
    expectContains(runtime_payload, "\"traffic_sign_inference_ms\":0.000000",
                   "Runtime deve zerar tempo de inferencia local.");
    expectContains(runtime_payload, "\"traffic_sign_dropped_frames\":0",
                   "Runtime deve zerar descartes da inferencia local.");
    expectContains(runtime_payload, "\"sign_result_age_ms\":-1",
                   "Runtime deve sinalizar ausencia de resultado local de placas.");
    expect(findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"").empty(),
           "Servico de segmentacao nao deve gerar telemetria local de placas.");
}

void testServiceKeepsStreamResponsiveWhenTelemetryPublisherIsSlow() {
    const auto image_path = createStaticTestImage("road_segmentation_service_slow_telemetry.png");
    const auto vision_config_path = createVisionConfig("image", image_path, 30.0, 30.0);

    autoctrl::AutonomousControlService control_service;
    CapturedServiceOutput output;
    std::atomic<bool> telemetry_callback_running{false};
    services::RoadSegmentationService service(
        vision_config_path.string(), &control_service,
        [&](const std::string &payload) {
            telemetry_callback_running.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(220));
            std::lock_guard<std::mutex> lock(output.mutex);
            output.telemetry.push_back(payload);
            telemetry_callback_running.store(false);
        },
        [&](vision::VisionDebugViewId view, const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.frames.emplace_back(view, payload);
        },
        [] {
            return vision::VisionDebugViewSet{vision::VisionDebugViewId::Raw,
                                              vision::VisionDebugViewId::Dashboard};
        });

    service.start();
    const bool stream_ready = waitForCondition(
        [&] { return telemetry_callback_running.load() && frameCount(output) >= 2; }, 1000);
    service.stop();

    expect(stream_ready,
           "Stream deve continuar produzindo frames mesmo com publicador de telemetria lento.");
}

TestRegistrar road_segmentation_service_core_test(
    "road_segmentation_service_publishes_core_runtime_and_stream_without_local_traffic_sign_pipeline",
    testServicePublishesCoreTelemetryAndStreamWithoutLocalTrafficSignPipeline);
TestRegistrar road_segmentation_service_stream_test(
    "road_segmentation_service_keeps_stream_responsive_when_telemetry_publisher_is_slow",
    testServiceKeepsStreamResponsiveWhenTelemetryPublisherIsSlow);

} // namespace
