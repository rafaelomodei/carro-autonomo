cmake_minimum_required(VERSION 3.16)
project(road_segmentation_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui calib3d)

add_library(road_segmentation_lab_core
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

target_include_directories(road_segmentation_lab_core
    PUBLIC
        src
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(road_segmentation_lab_core
    PUBLIC
        ${OpenCV_LIBS}
)

add_executable(road_segmentation_lab
    src/main.cpp
)

target_link_libraries(road_segmentation_lab
    PRIVATE
        road_segmentation_lab_core
)

enable_testing()

add_executable(road_segmentation_lab_tests
    tests/RoadSegmentationPipelineTests.cpp
)

target_link_libraries(road_segmentation_lab_tests
    PRIVATE
        road_segmentation_lab_core
)

add_test(
    NAME road_segmentation_lab_tests
    COMMAND road_segmentation_lab_tests
)
