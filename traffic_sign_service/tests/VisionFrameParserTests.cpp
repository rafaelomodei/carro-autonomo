#include "TestRegistry.hpp"
#include "vision/VisionFrame.hpp"

namespace {

using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::tests::expectEqual;
using traffic_sign_service::vision::parseVisionFrameMessage;

void testValidRawVisionFrameParses() {
    const std::string payload =
        R"({"type":"vision.frame","view":"raw","timestamp_ms":1,"mime":"image/jpeg","width":4,"height":4,"data":"aGVsbG8="})";

    std::string error;
    const auto parsed = parseVisionFrameMessage(payload, &error);

    expect(parsed.has_value(), "Payload raw valido deve ser aceito.");
    expect(error.empty(), "Payload valido nao deve preencher erro.");
    expectEqual(parsed->view, "raw", "View raw deve ser preservada.");
}

void testInvalidJsonIsRejected() {
    std::string error;
    const auto parsed = parseVisionFrameMessage("{", &error);

    expect(!parsed.has_value(), "JSON quebrado deve ser rejeitado.");
    expect(!error.empty(), "Erro deve descrever a falha de parsing.");
}

void testInvalidBase64IsRejected() {
    const std::string payload =
        R"({"type":"vision.frame","view":"raw","timestamp_ms":1,"mime":"image/jpeg","width":4,"height":4,"data":"@@@"})";

    std::string error;
    const auto parsed = parseVisionFrameMessage(payload, &error);

    expect(!parsed.has_value(), "Base64 invalido deve ser rejeitado.");
    expect(!error.empty(), "Erro deve explicar o base64 invalido.");
}

void testUnsupportedViewIsRejected() {
    const std::string payload =
        R"({"type":"vision.frame","view":"annotated","timestamp_ms":1,"mime":"image/jpeg","width":4,"height":4,"data":"aGVsbG8="})";

    std::string error;
    const auto parsed = parseVisionFrameMessage(payload, &error);

    expect(!parsed.has_value(), "Views fora de raw nao devem entrar no servico.");
    expect(!error.empty(), "Erro deve indicar que a view nao e suportada.");
}

TestRegistrar vision_frame_valid_test("vision_frame_parser_accepts_raw_messages",
                                      testValidRawVisionFrameParses);
TestRegistrar vision_frame_json_test("vision_frame_parser_rejects_invalid_json",
                                     testInvalidJsonIsRejected);
TestRegistrar vision_frame_base64_test("vision_frame_parser_rejects_invalid_base64",
                                       testInvalidBase64IsRejected);
TestRegistrar vision_frame_view_test("vision_frame_parser_rejects_unsupported_view",
                                     testUnsupportedViewIsRejected);

} // namespace
