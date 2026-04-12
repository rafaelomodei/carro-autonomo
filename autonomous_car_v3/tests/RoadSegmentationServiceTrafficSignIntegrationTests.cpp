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
        ts::TrafficSignInferenceInput input;
        input.frame = frame;
        input.full_frame_size = frame.size();
        return detect(input, timestamp_ms);
    }

    ts::TrafficSignFrameResult detect(const ts::TrafficSignInferenceInput &input,
                                      std::int64_t timestamp_ms) override {
        const cv::Size full_frame_size =
            input.full_frame_size.area() > 0 ? input.full_frame_size : input.frame.size();
        ts::TrafficSignFrameResult frame_result = ts::makeTrafficSignFrameResult(
            state_,
            input.roi.value_or(
                ts::buildTrafficSignRoi(full_frame_size, 0.55, 1.0, 0.08, 0.72)),
            timestamp_ms,
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

struct CapturedTrafficSignInput {
    cv::Size frame_size;
    cv::Size full_frame_size;
    ts::TrafficSignRoi roi;
    bool seen{false};
};

class InspectingTrafficSignDetector : public FakeTrafficSignDetector {
public:
    explicit InspectingTrafficSignDetector(CapturedTrafficSignInput *captured_input)
        : FakeTrafficSignDetector(ts::TrafficSignDetectorState::Idle),
          captured_input_(captured_input) {}

    ts::TrafficSignFrameResult detect(const ts::TrafficSignInferenceInput &input,
                                      std::int64_t timestamp_ms) override {
        if (captured_input_) {
            captured_input_->frame_size = input.frame.size();
            captured_input_->full_frame_size = input.full_frame_size;
            captured_input_->roi =
                input.roi.value_or(ts::buildTrafficSignRoi(input.full_frame_size, 0.55, 1.0,
                                                           0.08, 0.72));
            captured_input_->seen = true;
        }
        return FakeTrafficSignDetector::detect(input, timestamp_ms);
    }

private:
    CapturedTrafficSignInput *captured_input_;
};

class SlowFakeTrafficSignDetector : public FakeTrafficSignDetector {
public:
    SlowFakeTrafficSignDetector(ts::TrafficSignDetectorState state, int sleep_ms,
                                std::string last_error = {})
        : FakeTrafficSignDetector(state, std::move(last_error)), sleep_ms_(sleep_ms) {}

    ts::TrafficSignFrameResult detect(const cv::Mat &frame,
                                      std::int64_t timestamp_ms) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms_));
        return FakeTrafficSignDetector::detect(frame, timestamp_ms);
    }

private:
    int sleep_ms_;
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
        cv::rectangle(frame, {250, 30}, {295, 75}, {255, 255, 255}, cv::FILLED);
        writer.write(frame);
    }

    writer.release();
    return video_path;
}

std::filesystem::path createVisionConfig(const std::string &source_mode,
                                         const std::filesystem::path &source_path,
                                         const std::filesystem::path &traffic_sign_path,
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
        "TRAFFIC_SIGN_TARGET_FPS=4\n"
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

std::size_t frameCount(const CapturedServiceOutput &output) {
    std::lock_guard<std::mutex> lock(output.mutex);
    return output.frames.size();
}

std::size_t countTelemetryByType(const CapturedServiceOutput &output, const std::string &type) {
    std::lock_guard<std::mutex> lock(output.mutex);
    std::size_t matches = 0;
    for (const auto &payload : output.telemetry) {
        if (payload.find(type) != std::string::npos) {
            ++matches;
        }
    }
    return matches;
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

long long extractIntegerField(const std::string &payload, const std::string &field) {
    const std::string token = "\"" + field + "\":";
    const auto position = payload.find(token);
    if (position == std::string::npos) {
        return -1;
    }

    const auto number_start = position + token.size();
    const auto number_end = payload.find_first_not_of("0123456789-", number_start);
    return std::stoll(payload.substr(number_start, number_end - number_start));
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
            const bool has_road_telemetry =
                !findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"").empty();
            const bool has_traffic_telemetry =
                !findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"")
                     .empty();

            bool has_annotated = false;
            bool has_dashboard = false;
            {
                std::lock_guard<std::mutex> lock(output.mutex);
                for (const auto &frame : output.frames) {
                    has_annotated =
                        has_annotated || frame.first == vision::VisionDebugViewId::Annotated;
                    has_dashboard =
                        has_dashboard || frame.first == vision::VisionDebugViewId::Dashboard;
                }
            }

            return has_road_telemetry && has_traffic_telemetry && has_annotated && has_dashboard;
        },
        4000);
    service.stop();

    expect(finished, "Servico deve publicar telemetria e frames esperados.");
    return output;
}

void testServiceKeepsPublishingWhenTrafficSignDisabled() {
    const auto image_path = createStaticTestImage("road_segmentation_service_disabled.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_disabled.env",
        "TRAFFIC_SIGN_ENABLED=false\n");
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

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
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

    const CapturedServiceOutput output = runServiceOnce(
        vision_config_path,
        [](const ts::TrafficSignConfig &) {
            return std::make_unique<FakeTrafficSignDetector>(ts::TrafficSignDetectorState::Idle);
        });

    const std::string traffic_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"");

    expectContains(traffic_payload, "\"detector_state\":\"confirmed\"",
                   "Filtro temporal deve confirmar a deteccao fake.");
    expectContains(traffic_payload, "\"sign_id\":\"stop\"",
                   "Payload deve carregar o sign_id confirmado.");
    expectContains(traffic_payload, "\"left_ratio\":0.550000",
                   "Telemetria deve expor a ROI livre.");
}

void testServiceBuildsTrafficSignRoiFromOriginalFrame() {
    const auto image_path = createStaticTestImage("road_segmentation_service_roi_origin.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_roi_origin.env",
        "TRAFFIC_SIGN_ENABLED=true\n"
        "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES=1\n"
        "TRAFFIC_SIGN_ROI_LEFT_RATIO=0.50\n"
        "TRAFFIC_SIGN_ROI_RIGHT_RATIO=0.90\n"
        "TRAFFIC_SIGN_ROI_TOP_RATIO=0.10\n"
        "TRAFFIC_SIGN_ROI_BOTTOM_RATIO=0.70\n");
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

    CapturedTrafficSignInput captured_input;
    const CapturedServiceOutput output = runServiceOnce(
        vision_config_path,
        [&captured_input](const ts::TrafficSignConfig &) {
            return std::make_unique<InspectingTrafficSignDetector>(&captured_input);
        });

    const std::string traffic_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"");

    expect(captured_input.seen, "Detector fake deve receber input de inferencia.");
    expect(captured_input.full_frame_size == cv::Size(320, 240),
           "Inferencia deve manter o tamanho do frame original.");
    expect(captured_input.roi.frame_rect == cv::Rect(160, 24, 128, 144),
           "ROI de sinalizacao deve ser calculada no frame original completo.");
    expect(captured_input.frame_size == captured_input.roi.frame_rect.size(),
           "Frame entregue ao detector deve ser o recorte exato da ROI.");
    expectContains(traffic_payload, "\"left_ratio\":0.500000",
                   "Telemetria deve refletir a ROI configurada.");
    expectContains(traffic_payload, "\"right_ratio\":0.900000",
                   "Telemetria deve refletir a ROI configurada.");
}

void testServicePublishesErrorWithoutStoppingSegmentation() {
    const auto image_path = createStaticTestImage("road_segmentation_service_error.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_error.env",
        "TRAFFIC_SIGN_ENABLED=true\n");
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

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

void testServiceKeepsCoreResponsiveWhenTrafficSignInferenceIsSlow() {
    const auto image_path = createStaticTestImage("road_segmentation_service_slow_sign.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_slow_sign.env",
        "TRAFFIC_SIGN_ENABLED=true\n"
        "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES=1\n");
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

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
        {}, "AutonomousCar Test Window",
        [](const ts::TrafficSignConfig &) {
            return std::make_unique<SlowFakeTrafficSignDetector>(
                ts::TrafficSignDetectorState::Idle, 450);
        });

    const auto started_at = std::chrono::steady_clock::now();
    service.start();
    const bool core_ready = waitForCondition(
        [&output] {
            return !findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"").empty() &&
                   frameCount(output) >= 2;
        },
        350);
    const auto core_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - started_at)
                                     .count();
    const bool traffic_ready = waitForCondition(
        [&output] {
            return !findTelemetryByType(output, "\"type\":\"telemetry.traffic_sign_detection\"")
                        .empty();
        },
        1500);
    service.stop();

    expect(core_ready,
           "Telemetria principal e stream devem aparecer antes do detector lento concluir.");
    expect(core_elapsed_ms < 400,
           "Caminho critico nao deve ficar bloqueado pela inferencia lenta de sinalizacao.");
    expect(traffic_ready,
           "Telemetria de sinalizacao deve seguir chegando de forma assincorna.");
}

void testServiceDoesNotPublishFramesWhenNoViewsAreSubscribed() {
    const auto image_path = createStaticTestImage("road_segmentation_service_no_views.png");
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_no_views.env",
        "TRAFFIC_SIGN_ENABLED=false\n");
    const auto vision_config_path = createVisionConfig("image", image_path, traffic_sign_path);

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
        [] { return vision::VisionDebugViewSet{}; });

    service.start();
    const bool road_ready = waitForCondition(
        [&output] {
            return !findTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"").empty();
        },
        1000);
    service.stop();

    expect(road_ready, "Segmentacao deve seguir ativa sem subscribers de stream.");
    expect(frameCount(output) == 0, "Nao deve haver frames publicados sem views inscritas.");
}

void testServiceDropsSlowStreamBacklogWithoutBlockingCore() {
    const auto video_path = createVideoTestFile("road_segmentation_service_slow_stream.avi", 90);
    const auto traffic_sign_path = writeTextFile(
        "road_segmentation_service_slow_stream.env",
        "TRAFFIC_SIGN_ENABLED=false\n");
    const auto vision_config_path =
        createVisionConfig("video", video_path, traffic_sign_path, 60.0, 30.0);

    CapturedServiceOutput output;
    services::RoadSegmentationService service(
        vision_config_path.string(), nullptr,
        [&](const std::string &payload) {
            std::lock_guard<std::mutex> lock(output.mutex);
            output.telemetry.push_back(payload);
        },
        [&](vision::VisionDebugViewId view, const std::string &payload) {
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            std::lock_guard<std::mutex> lock(output.mutex);
            output.frames.emplace_back(view, payload);
        },
        [] {
            return vision::VisionDebugViewSet{vision::VisionDebugViewId::Annotated,
                                              vision::VisionDebugViewId::Dashboard};
        });

    service.start();
    const bool runtime_ready = waitForCondition(
        [&output] {
            const std::string runtime_payload =
                findTelemetryByType(output, "\"type\":\"telemetry.vision_runtime\"");
            return !runtime_payload.empty() &&
                   extractIntegerField(runtime_payload, "stream_dropped_frames") > 0;
        },
        4000);
    service.stop();

    const std::string runtime_payload =
        findTelemetryByType(output, "\"type\":\"telemetry.vision_runtime\"");
    expect(runtime_ready, "Runtime deve refletir backlog descartado quando o stream ficar lento.");
    expect(countTelemetryByType(output, "\"type\":\"telemetry.road_segmentation\"") >= 2,
           "Core deve continuar emitindo segmentacao mesmo com stream lento.");
    expect(extractIntegerField(runtime_payload, "stream_dropped_frames") > 0,
           "Telemetria de runtime deve indicar descarte de frames do stream.");
}

TestRegistrar road_service_disabled_test(
    "road_segmentation_service_keeps_publishing_when_traffic_sign_disabled",
    testServiceKeepsPublishingWhenTrafficSignDisabled);
TestRegistrar road_service_confirmed_test(
    "road_segmentation_service_publishes_confirmed_traffic_sign_telemetry_with_fake_detector",
    testServicePublishesConfirmedTrafficSignTelemetryWithFakeDetector);
TestRegistrar road_service_roi_origin_test(
    "road_segmentation_service_builds_traffic_sign_roi_from_original_frame",
    testServiceBuildsTrafficSignRoiFromOriginalFrame);
TestRegistrar road_service_error_test(
    "road_segmentation_service_publishes_error_without_stopping_segmentation",
    testServicePublishesErrorWithoutStoppingSegmentation);
TestRegistrar road_service_slow_sign_test(
    "road_segmentation_service_keeps_core_responsive_when_traffic_sign_inference_is_slow",
    testServiceKeepsCoreResponsiveWhenTrafficSignInferenceIsSlow);
TestRegistrar road_service_no_views_test(
    "road_segmentation_service_does_not_publish_frames_when_no_views_are_subscribed",
    testServiceDoesNotPublishFramesWhenNoViewsAreSubscribed);
TestRegistrar road_service_slow_stream_test(
    "road_segmentation_service_drops_slow_stream_backlog_without_blocking_core",
    testServiceDropsSlowStreamBacklogWithoutBlockingCore);

} // namespace
