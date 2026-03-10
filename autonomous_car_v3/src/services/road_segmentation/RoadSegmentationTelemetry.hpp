#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pipeline/RoadSegmentationResult.hpp"

namespace autonomous_car::services::road_segmentation {

std::string buildRoadSegmentationTelemetryJson(
    const road_segmentation_lab::pipeline::RoadSegmentationResult &result,
    std::string_view source, std::int64_t timestamp_ms);

} // namespace autonomous_car::services::road_segmentation
