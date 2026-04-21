cmake_minimum_required(VERSION 3.16)
project(traffic_sign_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(nlohmann_json REQUIRED)

find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/client.hpp REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)

set(
    TRAFFIC_SIGN_SERVICE_MODEL_ZIP
    "${CMAKE_CURRENT_SOURCE_DIR}/../edgeImpulse/tcc-pare-direita-esquerda-cam-raspberry-v15.zip"
    CACHE FILEPATH
    "Path to the Edge Impulse model zip"
)

find_program(UNZIP_EXECUTABLE unzip REQUIRED)

set(EDGE_IMPULSE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/edge_impulse_model")
file(MAKE_DIRECTORY "${EDGE_IMPULSE_GENERATED_DIR}")

execute_process(
    COMMAND ${UNZIP_EXECUTABLE} -oq ${TRAFFIC_SIGN_SERVICE_MODEL_ZIP} -d ${EDGE_IMPULSE_GENERATED_DIR}
    RESULT_VARIABLE EDGE_IMPULSE_UNZIP_RESULT
)

if (NOT EDGE_IMPULSE_UNZIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Falha ao extrair o modelo Edge Impulse em ${EDGE_IMPULSE_GENERATED_DIR}")
endif ()

set(EI_MODEL_DIR "${EDGE_IMPULSE_GENERATED_DIR}")
set(EI_SDK_DIR "${EI_MODEL_DIR}/edge-impulse-sdk")

file(GLOB_RECURSE EI_CPP_SOURCES
    "${EI_SDK_DIR}/*.cpp"
    "${EI_SDK_DIR}/*.cc"
)

list(FILTER EI_CPP_SOURCES EXCLUDE REGEX "/edge-impulse-sdk/cmake/")
list(FILTER EI_CPP_SOURCES EXCLUDE REGEX "/edge-impulse-sdk/porting/(ambiq|arduino|brickml|espressif|himax|himax-we2|iar|infineon-psoc62|mbed|mingw32|particle|raspberry|renesas-ra|seeed-vision-ai|silabs|sony|stm32-cubeai|synaptics|ti|zephyr)/")

file(GLOB_RECURSE EI_CMSIS_SOURCES
    "${EI_SDK_DIR}/CMSIS/DSP/Source/TransformFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/CommonTables/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/BasicMathFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/ComplexMathFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/FastMathFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/SupportFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/MatrixFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/DSP/Source/StatisticsFunctions/*.c"
    "${EI_SDK_DIR}/CMSIS/NN/Source/*.c"
)

set(EI_MODEL_SOURCES
    "${EI_MODEL_DIR}/tflite-model/tflite_learn_3_compiled.cpp"
    "${EI_SDK_DIR}/tensorflow/lite/c/common.c"
)

set(TRAFFIC_SIGN_SERVICE_CORE_SOURCES
    src/app/TrafficSignService.cpp
    src/config/ServiceConfig.cpp
    src/domain/TrafficSignal.cpp
    src/edge_impulse/LabelMapping.cpp
    src/policy/DetectionPolicy.cpp
    src/visualization/FramePreviewWindow.cpp
    src/vision/Base64.cpp
    src/vision/FrameDecoder.cpp
    src/vision/VisionFrame.cpp
)

add_library(traffic_sign_service_core STATIC
    ${TRAFFIC_SIGN_SERVICE_CORE_SOURCES}
)

target_include_directories(traffic_sign_service_core
    PUBLIC
        src
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(traffic_sign_service_core
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
)

add_library(traffic_sign_service_transport STATIC
    src/transport/VehicleWebSocketClient.cpp
)

target_compile_definitions(traffic_sign_service_transport PRIVATE ASIO_STANDALONE)

target_include_directories(traffic_sign_service_transport
    PUBLIC
        src
        ${WEBSOCKETPP_INCLUDE_DIR}
        ${ASIO_INCLUDE_DIR}
)

target_link_libraries(traffic_sign_service_transport
    PUBLIC
        traffic_sign_service_core
        Threads::Threads
)

add_library(traffic_sign_service_edge_impulse STATIC
    src/edge_impulse/EdgeImpulseClassifier.cpp
    ${EI_MODEL_SOURCES}
    ${EI_CPP_SOURCES}
    ${EI_CMSIS_SOURCES}
)

target_include_directories(traffic_sign_service_edge_impulse
    PUBLIC
        src
        ${EI_MODEL_DIR}
        ${EI_SDK_DIR}
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(traffic_sign_service_edge_impulse
    PUBLIC
        traffic_sign_service_core
        ${OpenCV_LIBS}
)

add_executable(traffic_sign_service
    src/main.cpp
)

target_link_libraries(traffic_sign_service
    PRIVATE
        traffic_sign_service_core
        traffic_sign_service_transport
        traffic_sign_service_edge_impulse
)

enable_testing()

add_executable(traffic_sign_service_tests
    tests/TestMain.cpp
    tests/DetectionPolicyTests.cpp
    tests/LabelMappingTests.cpp
    tests/LatestFrameStoreTests.cpp
    tests/ServiceConfigTests.cpp
    tests/TrafficSignServiceTests.cpp
    tests/VisionFrameParserTests.cpp
)

target_link_libraries(traffic_sign_service_tests
    PRIVATE
        traffic_sign_service_core
)

add_test(
    NAME traffic_sign_service_tests
    COMMAND traffic_sign_service_tests
)
