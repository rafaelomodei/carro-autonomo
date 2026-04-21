#include <filesystem>
#include <fstream>

#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "config/ServiceConfig.hpp"
#include "edge_impulse/EimProcessClassifier.hpp"

namespace {

using traffic_sign_service::TrafficSignalId;
using traffic_sign_service::TrafficSignDetectorState;
using traffic_sign_service::TrafficSignInferenceInput;
using traffic_sign_service::config::ServiceConfig;
using traffic_sign_service::edge_impulse::EimProcessClassifier;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

std::filesystem::path writeExecutable(const std::string &filename,
                                      const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::ofstream file(path);
    file << contents;
    file.close();
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_exec | std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace);
    return path;
}

TrafficSignInferenceInput makeInput() {
    TrafficSignInferenceInput input;
    input.frame = cv::Mat(120, 160, CV_8UC3, cv::Scalar(0, 0, 255));
    input.full_frame_size = input.frame.size();
    return input;
}

void testEimClassifierLoadsPrintInfoAndParsesKnownLabel() {
    const auto script = writeExecutable(
        "traffic_sign_service_fake_eim_ok.sh",
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == \"--print-info\" ]]; then\n"
        "  echo '{\"success\":true,\"id\":1,\"model_parameters\":{\"model_type\":\"object detection\",\"image_input_width\":32,\"image_input_height\":32,\"labels\":[\"Parada Obrigatoria sign\",\"Mystery sign\"]}}'\n"
        "  exit 0\n"
        "fi\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":2}'\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":3,\"result\":{\"bounding_boxes\":[{\"label\":\"Parada Obrigatoria sign\",\"value\":0.91,\"x\":4,\"y\":5,\"width\":10,\"height\":12}]}}'\n");

    ServiceConfig config;
    config.edge_impulse_backend = traffic_sign_service::EdgeImpulseBackend::Eim;
    config.edge_impulse_eim_path = script.string();
    config.eim_print_info_on_start = false;
    EimProcessClassifier classifier(config);

    const auto result = classifier.detect(makeInput(), 1234);

    expect(classifier.inputSize() == cv::Size(32, 32),
           "Print-info deve preencher o tamanho de entrada.");
    expect(!classifier.modelLabels().empty(), "Print-info deve preencher as labels.");
    expect(!result.raw_detections.empty(), "Resposta valida deve gerar deteccao.");
    expect(result.raw_detections.front().signal_id == TrafficSignalId::Stop,
           "Label conhecida deve mapear para stop.");
}

void testEimClassifierMapsUnknownLabelToUnknown() {
    const auto script = writeExecutable(
        "traffic_sign_service_fake_eim_unknown.sh",
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == \"--print-info\" ]]; then\n"
        "  echo '{\"success\":true,\"id\":1,\"model_parameters\":{\"model_type\":\"object detection\",\"image_input_width\":32,\"image_input_height\":32,\"labels\":[\"Mystery sign\"]}}'\n"
        "  exit 0\n"
        "fi\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":2}'\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":3,\"result\":{\"bounding_boxes\":[{\"label\":\"Mystery sign\",\"value\":0.88,\"x\":1,\"y\":2,\"width\":6,\"height\":7}]}}'\n");

    ServiceConfig config;
    config.edge_impulse_backend = traffic_sign_service::EdgeImpulseBackend::Eim;
    config.edge_impulse_eim_path = script.string();
    config.eim_print_info_on_start = false;
    EimProcessClassifier classifier(config);

    const auto result = classifier.detect(makeInput(), 1234);
    expect(!result.raw_detections.empty(), "Resposta com label desconhecida deve ser preservada.");
    expect(result.raw_detections.front().signal_id == TrafficSignalId::Unknown,
           "Label desconhecida deve cair em unknown.");
}

void testEimClassifierTurnsBackendErrorsIntoErrorResults() {
    const auto script = writeExecutable(
        "traffic_sign_service_fake_eim_error.sh",
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == \"--print-info\" ]]; then\n"
        "  echo '{\"success\":true,\"id\":1,\"model_parameters\":{\"model_type\":\"object detection\",\"image_input_width\":32,\"image_input_height\":32,\"labels\":[\"Parada Obrigatoria sign\"]}}'\n"
        "  exit 0\n"
        "fi\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":2}'\n"
        "read -r line\n"
        "echo '{\"success\":false,\"id\":3,\"error\":\"falha fake\"}'\n");

    ServiceConfig config;
    config.edge_impulse_backend = traffic_sign_service::EdgeImpulseBackend::Eim;
    config.edge_impulse_eim_path = script.string();
    config.eim_print_info_on_start = false;
    EimProcessClassifier classifier(config);

    const auto result = classifier.detect(makeInput(), 1234);
    expect(result.detector_state == TrafficSignDetectorState::Error,
           "Erro do backend deve virar resultado error.");
    expect(!result.last_error.empty(), "Resultado error deve carregar a mensagem do backend.");
}

void testEimClassifierAcceptsConstrainedObjectDetectionModelType() {
    const auto script = writeExecutable(
        "traffic_sign_service_fake_eim_constrained.sh",
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == \"--print-info\" ]]; then\n"
        "  echo '{\"success\":true,\"id\":1,\"model_parameters\":{\"model_type\":\"constrained_object_detection\",\"image_input_width\":196,\"image_input_height\":196,\"image_channel_count\":1,\"input_features_count\":38416,\"labels\":[\"Parada Obrigatoria sign\",\"Vire a esquerda sign\"]}}'\n"
        "  exit 0\n"
        "fi\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":2}'\n"
        "read -r line\n"
        "echo '{\"success\":true,\"id\":3,\"result\":{\"bounding_boxes\":[]}}'\n");

    ServiceConfig config;
    config.edge_impulse_backend = traffic_sign_service::EdgeImpulseBackend::Eim;
    config.edge_impulse_eim_path = script.string();
    config.eim_print_info_on_start = false;
    EimProcessClassifier classifier(config);

    const auto result = classifier.detect(makeInput(), 1234);
    expect(result.detector_state != TrafficSignDetectorState::Error,
           "Modelos constrained_object_detection devem ser aceitos pelo backend EIM.");
    expect(classifier.inputSize() == cv::Size(196, 196),
           "Print-info com constrained_object_detection deve preencher o input corretamente.");
}

TestRegistrar eim_print_info_test("eim_process_classifier_parses_print_info_and_detection",
                                  testEimClassifierLoadsPrintInfoAndParsesKnownLabel);
TestRegistrar eim_unknown_label_test("eim_process_classifier_maps_unknown_labels",
                                     testEimClassifierMapsUnknownLabelToUnknown);
TestRegistrar eim_error_test("eim_process_classifier_turns_backend_errors_into_error_results",
                             testEimClassifierTurnsBackendErrorsIntoErrorResults);
TestRegistrar eim_constrained_model_type_test(
    "eim_process_classifier_accepts_constrained_object_detection_model_type",
    testEimClassifierAcceptsConstrainedObjectDetectionModelType);

} // namespace
