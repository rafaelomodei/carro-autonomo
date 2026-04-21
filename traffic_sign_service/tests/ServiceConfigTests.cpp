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

void testConfigParsesFramePreviewFlag() {
    const auto config_path = writeTempConfig(
        "traffic_sign_service_config.env",
        "VEHICLE_WS_URL=ws://127.0.0.1:8080\n"
        "EDGE_IMPULSE_MODEL_ZIP=../../edgeImpulse/model.zip\n"
        "FRAME_PREVIEW_ENABLED=true\n");

    ServiceConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadServiceConfigFromFile(config_path.string(), config, &warnings);

    expect(loaded, "Config valida deve carregar com sucesso.");
    expect(config.frame_preview_enabled,
           "FRAME_PREVIEW_ENABLED=true deve habilitar o preview.");
}

void testConfigRejectsInvalidFramePreviewFlag() {
    const auto config_path = writeTempConfig(
        "traffic_sign_service_config_invalid.env",
        "VEHICLE_WS_URL=ws://127.0.0.1:8080\n"
        "EDGE_IMPULSE_MODEL_ZIP=../../edgeImpulse/model.zip\n"
        "FRAME_PREVIEW_ENABLED=talvez\n");

    ServiceConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadServiceConfigFromFile(config_path.string(), config, &warnings);

    expect(!loaded, "Flag booleana invalida deve falhar no parser.");
    expect(!warnings.empty(), "Parser deve retornar warning para flag booleana invalida.");
}

TestRegistrar service_config_preview_test("service_config_parses_frame_preview_flag",
                                          testConfigParsesFramePreviewFlag);
TestRegistrar service_config_preview_invalid_test(
    "service_config_rejects_invalid_frame_preview_flag",
    testConfigRejectsInvalidFramePreviewFlag);

} // namespace
