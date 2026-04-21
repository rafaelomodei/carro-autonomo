#include "TestRegistry.hpp"
#include "domain/TrafficSignTypes.hpp"

namespace {

using traffic_sign_service::TrafficSignBoundingBox;
using traffic_sign_service::buildTrafficSignRoi;
using traffic_sign_service::clampBoundingBox;
using traffic_sign_service::scaleBoundingBox;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;

void testBuildTrafficSignRoiGeneratesValidBounds() {
    const auto roi = buildTrafficSignRoi({320, 240}, 0.55, 1.0, 0.08, 0.72, true);

    expect(roi.frame_rect.x >= 0, "ROI deve iniciar dentro do frame.");
    expect(roi.frame_rect.y >= 0, "ROI deve iniciar dentro do frame.");
    expect(roi.frame_rect.width > 0, "ROI deve ter largura positiva.");
    expect(roi.frame_rect.height > 0, "ROI deve ter altura positiva.");
    expect(roi.frame_rect.x + roi.frame_rect.width <= 320,
           "ROI deve caber na largura do frame.");
    expect(roi.frame_rect.y + roi.frame_rect.height <= 240,
           "ROI deve caber na altura do frame.");
}

void testBoundingBoxesScaleAndClampCorrectly() {
    const TrafficSignBoundingBox model_box{8, 8, 16, 16};
    const auto scaled = scaleBoundingBox(model_box, {32, 32}, {160, 120});
    const auto clamped = clampBoundingBox({scaled.x, scaled.y, 400, 300}, {0, 0, 160, 120});

    expect(scaled.width == 80, "Bounding box deve escalar largura.");
    expect(scaled.height == 60, "Bounding box deve escalar altura.");
    expect(clamped.x >= 0 && clamped.y >= 0, "Clamp deve manter caixa dentro do frame.");
    expect(clamped.x + clamped.width <= 160, "Clamp deve respeitar largura do frame.");
    expect(clamped.y + clamped.height <= 120, "Clamp deve respeitar altura do frame.");
}

TestRegistrar traffic_sign_roi_test("traffic_sign_types_builds_valid_roi",
                                    testBuildTrafficSignRoiGeneratesValidBounds);
TestRegistrar traffic_sign_bbox_test("traffic_sign_types_scale_and_clamp_bounding_boxes",
                                     testBoundingBoxesScaleAndClampCorrectly);

} // namespace
