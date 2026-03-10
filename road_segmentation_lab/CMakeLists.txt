cmake_minimum_required(VERSION 3.16)
project(road_segmentation_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui calib3d)
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/road_segmentation
    ${CMAKE_CURRENT_BINARY_DIR}/shared_road_segmentation
)

add_executable(road_segmentation_lab
    src/main.cpp
)

target_link_libraries(road_segmentation_lab
    PRIVATE
        road_segmentation_core
)

enable_testing()

add_executable(road_segmentation_lab_tests
    tests/RoadSegmentationPipelineTests.cpp
)

target_link_libraries(road_segmentation_lab_tests
    PRIVATE
        road_segmentation_core
)

add_test(
    NAME road_segmentation_lab_tests
    COMMAND road_segmentation_lab_tests
)
