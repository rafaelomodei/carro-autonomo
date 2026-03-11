#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TestRegistry.hpp"
#include "services/road_segmentation/VisionRuntimeConfig.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::services::road_segmentation::VisionRuntimeConfig;
using autonomous_car::services::road_segmentation::VisionSourceMode;
using autonomous_car::services::road_segmentation::loadVisionRuntimeConfigFromFile;

std::filesystem::path writeTempFile(const std::string &name, const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

void testVisionRuntimeConfigLoadAndResolvePaths() {
    const auto config_dir = std::filesystem::temp_directory_path() / "autonomous_car_v3_vision_test";
    std::filesystem::create_directories(config_dir);

    const auto vision_path = config_dir / "vision.env";
    {
        std::ofstream file(vision_path);
        file << "VISION_SOURCE_MODE=video\n";
        file << "VISION_SOURCE_PATH=assets/input.mp4\n";
        file << "VISION_CAMERA_INDEX=2\n";
        file << "VISION_DEBUG_WINDOW_ENABLED=false\n";
        file << "VISION_TELEMETRY_MAX_FPS=15\n";
        file << "VISION_STREAM_MAX_FPS=6\n";
        file << "VISION_STREAM_JPEG_QUALITY=82\n";
        file << "VISION_SEGMENTATION_CONFIG_PATH=config/road_segmentation.env\n";
        file << "VISION_TRAFFIC_SIGN_CONFIG_PATH=config/traffic_sign_detection.env\n";
    }

    VisionRuntimeConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadVisionRuntimeConfigFromFile(vision_path.string(), config, &warnings);

    expect(loaded, "vision.env deve carregar com sucesso.");
    expect(config.source_mode == VisionSourceMode::Video,
           "Modo de fonte deve ser video.");
    expect(config.source_path == (config_dir / "assets/input.mp4").string(),
           "VISION_SOURCE_PATH relativo deve ser resolvido.");
    expect(config.camera_index == 2, "VISION_CAMERA_INDEX deve ser carregado.");
    expect(!config.debug_window_enabled, "VISION_DEBUG_WINDOW_ENABLED deve ser carregado.");
    expect(config.telemetry_max_fps == 15.0, "VISION_TELEMETRY_MAX_FPS deve ser carregado.");
    expect(config.stream_max_fps == 6.0, "VISION_STREAM_MAX_FPS deve ser carregado.");
    expect(config.stream_jpeg_quality == 82,
           "VISION_STREAM_JPEG_QUALITY deve ser carregado.");
    expect(config.segmentation_config_path ==
               (config_dir / "config/road_segmentation.env").string(),
           "VISION_SEGMENTATION_CONFIG_PATH relativo deve ser resolvido.");
    expect(config.traffic_sign_config_path ==
               (config_dir / "config/traffic_sign_detection.env").string(),
           "VISION_TRAFFIC_SIGN_CONFIG_PATH relativo deve ser resolvido.");
    expect(warnings.empty(), "Nao deve haver warnings para arquivo valido.");
}

void testVisionRuntimeConfigWarnsOnMissingPathForFileSource() {
    const auto vision_path = writeTempFile(
        "vision_runtime_warn.env",
        "VISION_SOURCE_MODE=image\n"
        "VISION_DEBUG_WINDOW_ENABLED=true\n");

    VisionRuntimeConfig config;
    std::vector<std::string> warnings;
    loadVisionRuntimeConfigFromFile(vision_path.string(), config, &warnings);

    expect(!warnings.empty(), "Arquivo sem caminho local deve gerar warning.");
}

TestRegistrar vision_config_test("vision_runtime_config_load_and_resolve_paths",
                                 testVisionRuntimeConfigLoadAndResolvePaths);
TestRegistrar vision_warning_test("vision_runtime_config_warns_on_missing_path_for_file_source",
                                  testVisionRuntimeConfigWarnsOnMissingPathForFileSource);

} // namespace
