#include "services/RoadSegmentationService.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <opencv2/highgui.hpp>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationPipeline.hpp"
#include "pipeline/stages/FrameSource.hpp"
#include "services/autonomous_control/AutonomousControlDebugRenderer.hpp"
#include "services/autonomous_control/AutonomousControlService.hpp"
#include "services/autonomous_control/AutonomousControlTelemetry.hpp"
#include "services/road_segmentation/RoadSegmentationTelemetry.hpp"
#include "services/road_segmentation/VisionRuntimeConfig.hpp"

namespace autonomous_car::services {
namespace {

namespace rsl = road_segmentation_lab;
namespace rs = autonomous_car::services::road_segmentation;
namespace autoctrl = autonomous_car::services::autonomous_control;

struct RuntimeState {
    rs::VisionRuntimeConfig vision_config;
    rsl::config::LabConfig segmentation_config;
    std::unique_ptr<rsl::pipeline::stages::FrameSource> source;
    std::string source_label;
};

void printWarnings(const char *scope, const std::vector<std::string> &warnings) {
    for (const auto &warning : warnings) {
        std::cerr << "[" << scope << "] " << warning << std::endl;
    }
}

std::int64_t currentTimestampMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

bool shouldPublishNow(double max_fps, std::chrono::steady_clock::time_point now,
                      std::chrono::steady_clock::time_point &last_publish) {
    if (max_fps <= 0.0) {
        return true;
    }

    if (last_publish == std::chrono::steady_clock::time_point::min()) {
        last_publish = now;
        return true;
    }

    const auto min_interval = std::chrono::duration<double>(1.0 / max_fps);
    if ((now - last_publish) >= min_interval) {
        last_publish = now;
        return true;
    }

    return false;
}

std::unique_ptr<rsl::pipeline::stages::FrameSource>
createFrameSource(const rs::VisionRuntimeConfig &config, std::vector<std::string> *warnings,
                  std::string *source_label) {
    auto build_camera_source = [&](int index, const std::string &reason)
        -> std::unique_ptr<rsl::pipeline::stages::FrameSource> {
        try {
            auto source = std::make_unique<rsl::pipeline::stages::FrameSource>(
                rsl::pipeline::stages::FrameSource::fromCameraIndex(index));
            if (source_label) {
                *source_label = source->description();
            }
            if (!reason.empty() && warnings) {
                warnings->push_back(reason);
            }
            return source;
        } catch (const std::exception &ex) {
            if (warnings) {
                warnings->push_back("Falha ao abrir camera fallback: " + std::string(ex.what()));
            }
            return nullptr;
        }
    };

    if (config.source_mode == rs::VisionSourceMode::Camera) {
        return build_camera_source(config.camera_index, {});
    }

    try {
        auto source = std::make_unique<rsl::pipeline::stages::FrameSource>(
            rsl::pipeline::stages::FrameSource::fromInputPath(config.source_path));
        if (source_label) {
            *source_label = source->description();
        }
        return source;
    } catch (const std::exception &ex) {
        const std::string warning =
            "Falha ao abrir fonte local (" + config.source_path + "): " + ex.what() +
            ". Tentando fallback para camera.";
        return build_camera_source(config.camera_index, warning);
    }
}

bool loadRuntimeState(const std::string &vision_config_path, RuntimeState &state) {
    state = RuntimeState{};

    std::vector<std::string> vision_warnings;
    rs::loadVisionRuntimeConfigFromFile(vision_config_path, state.vision_config, &vision_warnings);
    printWarnings("RoadSegmentationService/vision", vision_warnings);

    std::vector<std::string> segmentation_warnings;
    rsl::config::loadConfigFromFile(state.vision_config.segmentation_config_path,
                                    state.segmentation_config, &segmentation_warnings);
    printWarnings("RoadSegmentationService/segmentation", segmentation_warnings);

    std::vector<std::string> source_warnings;
    state.source = createFrameSource(state.vision_config, &source_warnings, &state.source_label);
    printWarnings("RoadSegmentationService/source", source_warnings);

    if (!state.source) {
        std::cerr << "[RoadSegmentationService] Nenhuma fonte de video disponivel." << std::endl;
        return false;
    }

    if (state.source_label.empty()) {
        state.source_label = state.source->description();
    }

    return true;
}

} // namespace

RoadSegmentationService::RoadSegmentationService(std::string vision_config_path,
                                                 autoctrl::AutonomousControlService *control_service,
                                                 TelemetryPublisher publisher,
                                                 ControlSink control_sink,
                                                 std::string window_name)
    : vision_config_path_(std::move(vision_config_path)),
      control_service_(control_service),
      publisher_(std::move(publisher)),
      control_sink_(std::move(control_sink)),
      window_name_(std::move(window_name)) {}

RoadSegmentationService::~RoadSegmentationService() { stop(); }

void RoadSegmentationService::start() {
    if (running_.load()) {
        return;
    }

    stop_requested_.store(false);
    worker_ = std::thread(&RoadSegmentationService::run, this);
}

void RoadSegmentationService::stop() {
    stop_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool RoadSegmentationService::isRunning() const noexcept { return running_.load(); }

void RoadSegmentationService::run() {
    running_.store(true);

    try {
        RuntimeState state;
        if (!loadRuntimeState(vision_config_path_, state)) {
            running_.store(false);
            return;
        }

        rsl::pipeline::RoadSegmentationPipeline pipeline(state.segmentation_config);
        autoctrl::AutonomousControlDebugRenderer renderer;

        bool paused = state.source->isStaticImage();
        bool step_once = true;
        bool need_reprocess = true;
        bool window_created = false;
        cv::Mat current_frame;
        cv::Mat current_dashboard;
        rsl::pipeline::RoadSegmentationResult current_result;
        autoctrl::AutonomousControlSnapshot current_control_snapshot;
        auto last_publish = std::chrono::steady_clock::time_point::min();

        if (state.vision_config.debug_window_enabled) {
            cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
            window_created = true;
        }

        while (!stop_requested_.load()) {
            const bool should_read_static_image =
                state.source->isStaticImage() && (need_reprocess || current_frame.empty());
            const bool should_read_dynamic_frame =
                !state.source->isStaticImage() && !need_reprocess &&
                (!paused || step_once || current_frame.empty());

            if (should_read_static_image || should_read_dynamic_frame) {
                if (!state.source->read(current_frame) || current_frame.empty()) {
                    std::cerr << "[RoadSegmentationService] Falha ao ler a fonte de video."
                              << std::endl;
                    break;
                }
            }

            if (need_reprocess || should_read_static_image || should_read_dynamic_frame) {
                current_result = pipeline.process(current_frame);
                const auto timestamp_ms = currentTimestampMs();

                if (control_service_) {
                    current_control_snapshot = control_service_->process(current_result, timestamp_ms);
                    if (control_sink_) {
                        control_sink_(current_control_snapshot);
                    }
                } else {
                    current_control_snapshot.timestamp_ms = timestamp_ms;
                }

                if (state.vision_config.debug_window_enabled) {
                    current_dashboard = renderer.render(current_result, state.segmentation_config,
                                                        state.source_label,
                                                        pipeline.calibrationStatus(),
                                                        current_control_snapshot);
                } else {
                    current_dashboard.release();
                }

                if (publisher_) {
                    const auto now = std::chrono::steady_clock::now();
                    if (shouldPublishNow(state.vision_config.telemetry_max_fps, now, last_publish)) {
                        publisher_(rs::buildRoadSegmentationTelemetryJson(
                            current_result, state.source_label, timestamp_ms));
                        if (control_service_) {
                            publisher_(autoctrl::buildAutonomousControlTelemetryJson(
                                current_control_snapshot));
                        }
                    }
                }

                step_once = false;
                need_reprocess = false;
            }

            if (state.vision_config.debug_window_enabled) {
                if (!window_created) {
                    cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
                    window_created = true;
                }

                if (!current_dashboard.empty()) {
                    cv::imshow(window_name_, current_dashboard);
                }

                const int key = cv::waitKey(state.source->isStaticImage() ? 30 : 10);
                switch (key) {
                case 27:
                case 'q':
                case 'Q':
                    stop_requested_.store(true);
                    break;
                case 'p':
                case 'P':
                    if (!state.source->isStaticImage()) {
                        paused = !paused;
                    }
                    break;
                case 'n':
                case 'N':
                    if (!state.source->isStaticImage() && paused) {
                        step_once = true;
                    }
                    break;
                case 'r':
                case 'R': {
                    RuntimeState reloaded_state;
                    if (loadRuntimeState(vision_config_path_, reloaded_state)) {
                        state = std::move(reloaded_state);
                        pipeline.updateConfig(state.segmentation_config);
                        paused = state.source->isStaticImage();
                        step_once = true;
                        need_reprocess = true;
                        current_frame.release();
                        current_dashboard.release();
                        last_publish = std::chrono::steady_clock::time_point::min();

                        if (!state.vision_config.debug_window_enabled && window_created) {
                            destroyWindow();
                            window_created = false;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            } else {
                if (window_created) {
                    destroyWindow();
                    window_created = false;
                }

                if (state.source->isStaticImage()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
            }
        }

        if (window_created) {
            destroyWindow();
        }
        if (control_service_) {
            control_service_->stopAutonomous(autoctrl::StopReason::ServiceStop);
            if (control_sink_) {
                control_sink_(control_service_->snapshot());
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "[RoadSegmentationService] Excecao: " << ex.what() << std::endl;
        destroyWindow();
        if (control_service_) {
            control_service_->stopAutonomous(autoctrl::StopReason::ServiceStop);
            if (control_sink_) {
                control_sink_(control_service_->snapshot());
            }
        }
    }

    running_.store(false);
}

void RoadSegmentationService::destroyWindow() const {
    if (!window_name_.empty()) {
        cv::destroyWindow(window_name_);
    }
}

} // namespace autonomous_car::services
