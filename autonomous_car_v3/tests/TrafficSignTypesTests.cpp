#include <string>

#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "services/traffic_sign_detection/TrafficSignTypes.hpp"

namespace {

using autonomous_car::tests::TestRegistrar;
using autonomous_car::tests::expect;
namespace ts = autonomous_car::services::traffic_sign_detection;

void testTrafficSignLabelNormalizationAndMapping() {
    expect(ts::normalizeModelLabel("Parada Obrigatoria sign") == "paradaobrigatoriasign",
           "Normalizacao deve remover espacos e pontuacao.");
    expect(ts::trafficSignIdFromModelLabel("Parada Obrigatoria sign") == ts::TrafficSignId::Stop,
           "Label de parada deve mapear para stop.");
    expect(ts::trafficSignIdFromModelLabel("turnleft") == ts::TrafficSignId::TurnLeft,
           "Label turnleft deve mapear para turn_left.");
    expect(ts::trafficSignIdFromModelLabel("turn_right?") == ts::TrafficSignId::TurnRight,
           "Label turnright deve mapear para turn_right.");
    expect(ts::trafficSignIdFromModelLabel("random class") == ts::TrafficSignId::Unknown,
           "Label desconhecida deve mapear para unknown.");
}

void testTrafficSignBoundingBoxRemap() {
    const ts::TrafficSignBoundingBox model_box{49, 49, 98, 98};
    const ts::TrafficSignBoundingBox roi_box =
        ts::scaleBoundingBox(model_box, {196, 196}, {196, 98});
    const ts::TrafficSignBoundingBox frame_box =
        ts::translateBoundingBox(roi_box, {100, 40});

    expect(roi_box.x == 49, "Bounding box no ROI deve manter proporcao em X.");
    expect(roi_box.y == 25, "Bounding box no ROI deve escalar Y.");
    expect(roi_box.width == 98, "Bounding box no ROI deve escalar width.");
    expect(roi_box.height == 49, "Bounding box no ROI deve escalar height.");
    expect(frame_box.x == 149 && frame_box.y == 65,
           "Bounding box no frame deve aplicar offset do ROI.");
}

void testTrafficSignFreeRoiBuildsRectFromFullFrame() {
    const ts::TrafficSignRoi roi =
        ts::buildTrafficSignRoi({640, 480}, 0.50, 0.75, 0.10, 0.80, false);

    expect(roi.frame_rect.x == 320, "ROI livre deve usar left_ratio no frame completo.");
    expect(roi.frame_rect.width == 160, "ROI livre deve usar right_ratio no frame completo.");
    expect(roi.frame_rect.y == 48, "ROI livre deve usar top_ratio no frame completo.");
    expect(roi.frame_rect.height == 336,
           "ROI livre deve usar bottom_ratio no frame completo.");
    expect(!roi.debug_roi_enabled, "Flag de debug da ROI deve ser preservada.");
    expect(roi.source_frame_size == cv::Size(640, 480),
           "ROI deve preservar o tamanho do frame de origem.");
    expect(ts::trafficSignRoiRightWidthRatio(roi) == 0.25,
           "Largura derivada da ROI deve permanecer disponivel.");
}

TestRegistrar traffic_sign_types_mapping_test("traffic_sign_types_label_mapping",
                                              testTrafficSignLabelNormalizationAndMapping);
TestRegistrar traffic_sign_bbox_remap_test("traffic_sign_types_bbox_remap",
                                           testTrafficSignBoundingBoxRemap);
TestRegistrar traffic_sign_free_roi_test("traffic_sign_types_builds_free_roi_from_full_frame",
                                         testTrafficSignFreeRoiBuildsRectFromFullFrame);

} // namespace
