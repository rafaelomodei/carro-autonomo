#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "TestRegistry.hpp"
#include "services/RoadSegmentationService.hpp"
#include "services/traffic_sign_detection/TrafficSignConfig.hpp"
#include "services/traffic_sign_detection/TrafficSignDetector.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::tests::expectContains;
namespace services = autonomous_car::services;
namespace ts = autonomous_car::services::traffic_sign_detection;
namespace vision = autonomous_car::services::vision;

struct CapturedServiceOutput {
    std::vector<std::string> telemetry;
    std::vector<std::pair<vision::VisionDebugViewId, std::string>> frames;
    mutable std::mutex mutex;

    CapturedServiceOutput() = default;
    CapturedServiceOutput(const CapturedServiceOutput &) = delete;
    CapturedServiceOutput &operator=(const CapturedServiceOutput &) = delete;

    CapturedServiceOutput(CapturedServiceOutput &&other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex);
        telemetry = std::move(other.telemetry);
        frames = std::move(other.frames);
    }

    CapturedServiceOutput &operator=(CapturedServiceOutput &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        std::scoped_lock lock(mutex, other.mutex);
        telemetry = std::move(other.telemetry);
        frames = std::move(other.frames);
        return *this;
    }
};

class FakeTrafficSignDetector : public ts::TrafficSignDetector {
public:
    explicit FakeTrafficSignDetector(ts::TrafficSignDetectorState state,
                                     std::string last_error = {})
        : state_(state), last_error_(std::move(last_error)) {}

    ts::TrafficSignFrameResult detect(const cv::Mat &frame,
                                      std::int64_t timestamp_ms) override {
        ts::TrafficSignFrameResult frame_result = ts::makeTrafficSignFrameResult(
            state_, ts::buildTrafficSignRoi(frame.size(), 0.45, 0.08, 0.72), timestamp_ms,
            last_error_);

        if (state_ == ts::TrafficSignDetectorState::Error) {
            return frame_result;
        }

        ts::TrafficSignDetection detection;
        detection.sign_id = ts::TrafficSignId::Stop;
        detection.model_label = "Parada Obrigatoria sign";
        detection.display_label = "Parada obrigatoria";
        detection.confidence_score = 0.97;
        detection.bbox_frame = {220, 40, 50, 50};
        detection.bbox_roi = {76, 20, 50, 50};
        detection.required_frames = 1;
        detection.last_seen_at_ms = timestamp_ms;
        frame_result.raw_detections.push_back(detection);
        frame_result.detector_state = ts::TrafficSignDetectorState::Idle;
        return frame_result;
    }

private:
    ts::TrafficSignDetectorState state_;
    std::string last_error_;
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
    cv::rectangle(frame, {250, 30}, {295, 75}, {255, 255, 255}, cv::FILLED);

    const auto image_path = std::filesystem::temp_directory_path() / name;
    cv::imwrite(image_path.string(), frame);
    return image_path;
}

std::filesystem::path createVisionConfig(const std::filesystem::path &image_path,
                                         const std::filesystem::path &traffic_sign_path) {
    const auto segmentation_path = std::filesystem::absolute(
        std::filesystem::path("autonomous_car_v3/config/road_segmentation.env"));

    return writeTextFile(
        "road_segmentation_service_vision.env",
        "VISION_SOURCE_MODE=image\n"
        "VISION_SOURCE_PATH=" + image_path.string() + "\n" +
            "VISION_DEBUG_WINDOW_ENABLED=false\n"
            "VISION_TELEMETRY_MAX_FPS=30\n"
            "VISION_STREAM_MAX_FPS=30\n"
            "VISION_STREAM_JPEG_QUALITY=70\n"
            "VISION_SEGMENTATION_CONFIG_PATH=" + segmentation_path.string() + "\n"
            "VISION_TRAFFIC_SIGN_CONFIG_PATH=" + traffic_sign_path.string() + "\n");
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

CapturedServiceOutput runServiceOnce(
    const std::filesystem::path &vision_config_path,
    services::RoadSegmentationService::TrafficSignDetectorFactory detector_factory = {}) {
    CapturedServiceOutput output;

    services::RoadSegmentationService service(
        vision_config_path.string(), nullptr,
        [&](const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.telemetry.push_back(payload);
        },
        [&](vision::VisionDebugViewId view, const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.frames.emplace_back(view, payload);
        },
        [] {
            return vision::VisionDebugViewSet{vision::VisionDebugViewId::Annotated,
                                              vision::VisionDebugViewId::Dashboard};
        },
        {}, "AutonomousCar Test Window", std::move(detector_factory));

    service.start();
    const bool finished = waitForCondition(
        [&output] {
            std::lock_guard<std::mutex> lock(output.mutex);

            bool has_road_telemetry = false;
            bool has_traffic_telemetry = false;
            bool has_annotated = false;
            bool has_dashboard = false;

            for (const auto &payload : output.telemetry) {
                has_road_telemetry =
                    has_road_telemetry ||
                    payload.find("\"type\":\"telemetry.road_segmentation\"") != std::string::npos;
                has_traffic_telemetry =
                    has_traffic_telemetry ||
                    payload.find("\"type\":\"telemetry.traffic_sign_detection\"") !=
                        std::string::npos;
            }

            for (const auto &frame : output.frames) {
                has_annotated = has_annotated || frame.first == vision::VisionDebugViewId::Annotated;
                has_dashboard = has_dashboard || frame.first == vision::VisionDebugViewId::Dashboard;
            }

            return has_road_telemetry && has_traffic_telemetry && has_annotated && has_dashboard;
        },
        4000);
    service.stop();

    expect(finished, "Servico deve publicar telemetria e frames esperados.");
    return output;
}

std::string findTelemetryByType(const CapturedServiceOutput &output, const std::string &type) {
    std::lock_guard<std::mutex> lock(output.mutex);
    for (const auto &payload : output.telemetry) {
        if (payload.find(type) != std::string::npos) {
            return payload;
        }
    }
    return {};
}

void testServiceKeepsPublishingWhenTrafficSignDisabled() {
    const auto image_path = createStaticTestImage("road_segmentation_service_disabled.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_disabled.env",
        "TRAFFIC_SIGN_ENABLED=false\n");
    const auto vision_config_path = createVisionConfig(image_path, traffic_sign_path);

    const CapturedServiceOutput output = runServiceOnce(vision_config_path);
    const std::string road_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"");
    const std::string traffic_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"");

    expect(!road_payload.empty(), "Telemetria de segmentacao deve continuar saindo.");
    expectContains(traffic_payload, "\"detector_state\":\"disabled\"",
                   "Telemetria de sinalizacao deve refletir detector desligado.");
}

void testServicePublishesConfirmedTrafficSignTelemetryWithFakeDetector() {
    const auto image_path = createStaticTestImage("road_segmentation_service_confirmed.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_confirmed.env",
        "TRAFFIC_SIGN_ENABLED=true\n"
        "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES=1\n");
    const auto vision_config_path = createVisionConfig(image_path, traffic_sign_path);

    const CapturedServiceOutput output = runServiceOnce(
        vision_config_path,
        [](const ts::TrafficSignConfig &) {
            return std::make_unique<FakeTrafficSignDetector>(
                ts::TrafficSignDetectorState::Idle);
        });

    const std::string traffic_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"");

    expectContains(traffic_payload, "\"detector_state\":\"confirmed\"",
                   "Filtro temporal deve confirmar a deteccao fake.");
    expectContains(traffic_payload, "\"sign_id\":\"stop\"",
                   "Payload deve carregar o sign_id confirmado.");
}

void testServicePublishesErrorWithoutStoppingSegmentation() {
    const auto image_path = createStaticTestImage("road_segmentation_service_error.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_error.env",
        "TRAFFIC_SIGN_ENABLED=true\n");
    const auto vision_config_path = createVisionConfig(image_path, traffic_sign_path);

    const CapturedServiceOutput output = runServiceOnce(
        vision_config_path,
        [](const ts::TrafficSignConfig &) {
            return std::make_unique<FakeTrafficSignDetector>(
                ts::TrafficSignDetectorState::Error, "Modelo invalido");
        });

    const std::string road_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"");
    const std::string traffic_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"");

    expect(!road_payload.empty(),
           "Segmentacao deve seguir publicando mesmo com erro no detector.");
    expectContains(traffic_payload, "\"detector_state\":\"error\"",
                   "Telemetria deve expor estado de erro.");
    expectContains(traffic_payload, "\"last_error\":\"Modelo invalido\"",
                   "Telemetria deve expor a causa do erro.");
}

TestRegistrar road_service_disabled_test(
    "road_segmentation_service_keeps_publishing_when_traffic_sign_disabled",
    testServiceKeepsPublishingWhenTrafficSignDisabled);
TestRegistrar road_service_confirmed_test(
    "road_segmentation_service_publishes_confirmed_traffic_sign_telemetry_with_fake_detector",
    testServicePublishesConfirmedTrafficSignTelemetryWithFakeDetector);
TestRegistrar road_service_error_test(
    "road_segmentation_service_publishes_error_without_stopping_segmentation",
    testServicePublishesErrorWithoutStoppingSegmentation);

} // namespace
