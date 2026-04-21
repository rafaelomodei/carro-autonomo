#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignConfig.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
using autonomous_car::services::traffic_sign_detection::TrafficSignConfig;
using autonomous_car::services::traffic_sign_detection::loadTrafficSignConfigFromFile;

std::filesystem::path writeTrafficSignConfigFile(const std::string &name,
                                                 const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

void testTrafficSignConfigLoad() {
    const auto path = writeTrafficSignConfigFile(
        "traffic_sign_config_valid.env",
        "TRAFFIC_SIGN_ENABLED=true\n"
        "TRAFFIC_SIGN_ROI_LEFT_RATIO=0.35\n"
        "TRAFFIC_SIGN_ROI_RIGHT_RATIO=0.92\n"
        "TRAFFIC_SIGN_ROI_TOP_RATIO=0.10\n"
        "TRAFFIC_SIGN_ROI_BOTTOM_RATIO=0.80\n"
        "TRAFFIC_SIGN_DEBUG_ROI_ENABLED=false\n"
        "TRAFFIC_SIGN_MIN_CONFIDENCE=0.72\n"
        "TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES=3\n"
        "TRAFFIC_SIGN_MAX_MISSED_FRAMES=4\n"
        "TRAFFIC_SIGN_MAX_RAW_DETECTIONS=5\n");

    TrafficSignConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadTrafficSignConfigFromFile(path.string(), config, &warnings);

    expect(loaded, "traffic_sign.env deve carregar com sucesso.");
    expect(config.enabled, "Detector deve ficar habilitado.");
    expect(config.roi_left_ratio == 0.35, "ROI left deve ser carregada.");
    expect(config.roi_right_ratio == 0.92, "ROI right deve ser carregada.");
    expect(config.roi_top_ratio == 0.10, "ROI top deve ser carregada.");
    expect(config.roi_bottom_ratio == 0.80, "ROI bottom deve ser carregada.");
    expect(!config.debug_roi_enabled, "Flag de debug da ROI deve ser carregada.");
    expect(config.min_confidence == 0.72, "Min confidence deve ser carregado.");
    expect(config.min_consecutive_frames == 3,
           "Min consecutive frames deve ser carregado.");
    expect(config.max_missed_frames == 4, "Max missed frames deve ser carregado.");
    expect(config.max_raw_detections == 5, "Max raw detections deve ser carregado.");
    expect(warnings.empty(), "Arquivo valido nao deve gerar warnings.");
}

void testTrafficSignConfigFallsBackToLegacyRightWidthRatio() {
    const auto path = writeTrafficSignConfigFile(
        "traffic_sign_config_legacy_roi.env",
        "TRAFFIC_SIGN_ROI_RIGHT_WIDTH_RATIO=0.50\n");

    TrafficSignConfig config;
    std::vector<std::string> warnings;
    const bool loaded = loadTrafficSignConfigFromFile(path.string(), config, &warnings);

    expect(loaded, "Config legado deve carregar com sucesso.");
    expect(config.roi_left_ratio == 0.50,
           "ROI left deve ser derivada da largura legacy.");
    expect(config.roi_right_ratio == 1.0,
           "ROI right deve ocupar a borda direita ao usar a chave legacy.");
    expect(warnings.empty(), "Fallback legado valido nao deve gerar warnings.");
}

void testTrafficSignConfigCorrectsInvalidRoi() {
    const auto path = writeTrafficSignConfigFile(
        "traffic_sign_config_invalid_roi.env",
        "TRAFFIC_SIGN_ROI_LEFT_RATIO=0.90\n"
        "TRAFFIC_SIGN_ROI_RIGHT_RATIO=0.80\n"
        "TRAFFIC_SIGN_ROI_TOP_RATIO=0.70\n"
        "TRAFFIC_SIGN_ROI_BOTTOM_RATIO=0.60\n");

    TrafficSignConfig config;
    std::vector<std::string> warnings;
    loadTrafficSignConfigFromFile(path.string(), config, &warnings);

    expect(config.roi_right_ratio > config.roi_left_ratio,
           "Right ratio deve ser corrigido para ficar acima do left ratio.");
    expect(config.roi_bottom_ratio > config.roi_top_ratio,
           "Bottom ratio deve ser corrigido para ficar acima do top ratio.");
    expect(!warnings.empty(), "Correcao de ROI invalido deve gerar warning.");
}

TestRegistrar traffic_sign_config_load_test("traffic_sign_config_load",
                                            testTrafficSignConfigLoad);
TestRegistrar traffic_sign_config_legacy_test(
    "traffic_sign_config_falls_back_to_legacy_right_width_ratio",
    testTrafficSignConfigFallsBackToLegacyRightWidthRatio);
TestRegistrar traffic_sign_config_roi_fix_test("traffic_sign_config_corrects_invalid_roi",
                                               testTrafficSignConfigCorrectsInvalidRoi);

} // namespace
