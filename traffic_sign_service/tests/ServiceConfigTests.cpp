#include <filesystem>
#include <fstream>

#include "TestRegistry.hpp"
#include "config/ServiceConfig.hpp"

namespace {

using traffic_sign_service::config::ServiceConfig;
using traffic_sign_service::config::loadServiceConfigFromFile;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

std::filesystem::path writeTempConfig(const std::string &filename, const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::ofstream file(path);
    file << contents;
    return path;
}

void testConfigParsesExtendedRuntimeOptions() {
    const auto config_path = writeTempConfig(
        "traffic_sign_service_config.env",
        "FRAME_SOURCE_MODE=camera\n"
        "EDGE_IMPULSE_BACKEND=embedded_cpp\n"
        "VEHICLE_WS_URL=ws://127.0.0.1:8080\n"
        "EDGE_IMPULSE_MODEL_ZIP=../../edgeImpulse/model.zip\n"
        "TRAFFIC_SIGN_ROI_LEFT_RATIO=0.60\n"
        "TRAFFIC_SIGN_PREPROCESS_GRAYSCALE=true\n"
        "FRAME_PREVIEW_ENABLED=true\n");

    ServiceConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadServiceConfigFromFile(config_path.string(), config, &warnings);

    expect(loaded, "Config valida deve carregar com sucesso.");
    expect(config.frame_preview_enabled,
           "FRAME_PREVIEW_ENABLED=true deve habilitar o preview.");
    expect(config.frame_source_mode == traffic_sign_service::FrameSourceMode::Camera,
           "FRAME_SOURCE_MODE=camera deve ser carregado.");
    expect(config.edge_impulse_backend == traffic_sign_service::EdgeImpulseBackend::EmbeddedCpp,
           "EDGE_IMPULSE_BACKEND=embedded_cpp deve ser carregado.");
    expect(config.traffic_sign_roi_left_ratio == 0.60,
           "TRAFFIC_SIGN_ROI_LEFT_RATIO deve ser carregado.");
    expect(config.traffic_sign_preprocess_grayscale,
           "TRAFFIC_SIGN_PREPROCESS_GRAYSCALE=true deve ser carregado.");
}

void testConfigRejectsInvalidFramePreviewFlag() {
    const auto config_path = writeTempConfig(
        "traffic_sign_service_config_invalid.env",
        "FRAME_SOURCE_MODE=camera\n"
        "EDGE_IMPULSE_BACKEND=embedded_cpp\n"
        "VEHICLE_WS_URL=ws://127.0.0.1:8080\n"
        "EDGE_IMPULSE_MODEL_ZIP=../../edgeImpulse/model.zip\n"
        "FRAME_PREVIEW_ENABLED=talvez\n");

    ServiceConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadServiceConfigFromFile(config_path.string(), config, &warnings);

    expect(!loaded, "Flag booleana invalida deve falhar no parser.");
    expect(!warnings.empty(), "Parser deve retornar warning para flag booleana invalida.");
}

void testConfigRequiresModeSpecificPaths() {
    const auto config_path = writeTempConfig(
        "traffic_sign_service_config_paths.env",
        "FRAME_SOURCE_MODE=image\n"
        "EDGE_IMPULSE_BACKEND=eim\n"
        "VEHICLE_WS_URL=ws://127.0.0.1:8080\n"
        "EDGE_IMPULSE_MODEL_ZIP=../../edgeImpulse/model.zip\n");

    ServiceConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadServiceConfigFromFile(config_path.string(), config, &warnings);

    expect(!loaded, "Config image/eim sem caminhos obrigatorios deve falhar.");
    expect(!warnings.empty(), "Falha deve reportar warnings de validacao.");
}

TestRegistrar service_config_preview_test("service_config_parses_extended_runtime_options",
                                          testConfigParsesExtendedRuntimeOptions);
TestRegistrar service_config_preview_invalid_test(
    "service_config_rejects_invalid_frame_preview_flag",
    testConfigRejectsInvalidFramePreviewFlag);
TestRegistrar service_config_paths_test("service_config_requires_mode_specific_paths",
                                        testConfigRequiresModeSpecificPaths);

} // namespace
