add_library(road_segmentation_core
    src/config/LabConfig.cpp
    src/pipeline/RoadSegmentationPipeline.cpp
    src/pipeline/BoundaryAnalyzer.cpp
    src/pipeline/stages/FrameSource.cpp
    src/pipeline/stages/ResizeStage.cpp
    src/pipeline/stages/UndistortStage.cpp
    src/pipeline/stages/RoiStage.cpp
    src/pipeline/stages/GaussianStage.cpp
    src/pipeline/stages/IlluminationStage.cpp
    src/pipeline/stages/SegmentationStage.cpp
    src/pipeline/stages/MorphologyStage.cpp
    src/render/DebugRenderer.cpp
)

target_include_directories(road_segmentation_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(road_segmentation_core
    PUBLIC
        ${OpenCV_LIBS}
)
