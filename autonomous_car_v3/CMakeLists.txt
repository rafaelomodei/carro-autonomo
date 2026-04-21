cmake_minimum_required(VERSION 3.16)
project(autonomous_car_v3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui calib3d)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/road_segmentation
    ${CMAKE_CURRENT_BINARY_DIR}/shared_road_segmentation
)

file(GLOB_RECURSE AUTONOMOUS_CAR_V3_TRAFFIC_SIGN_MODEL_SOURCES CONFIGURE_DEPENDS
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/classifier/*.cpp
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/dsp/*.cpp
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/porting/posix/*.cpp
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/tensorflow/*.cc
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/tensorflow/*.cpp
    third_party/edge_impulse/traffic_sign_model/edge-impulse-sdk/tensorflow/*.c
    third_party/edge_impulse/traffic_sign_model/tflite-model/*.cpp
)

add_library(autonomous_car_v3_traffic_sign_model STATIC
    ${AUTONOMOUS_CAR_V3_TRAFFIC_SIGN_MODEL_SOURCES}
)

target_include_directories(autonomous_car_v3_traffic_sign_model
    PUBLIC
        third_party/edge_impulse/traffic_sign_model
)

target_compile_definitions(autonomous_car_v3_traffic_sign_model
    PUBLIC
        EI_CLASSIFIER_IMAGE_ENABLED=1
        EI_CLASSIFIER_TFLITE_ENABLE=1
        EI_PORTING_POSIX=1
)

set(AUTONOMOUS_CAR_V3_COMMON_SOURCES
    src/config/ConfigurationManager.cpp
    src/controllers/CommandDispatcher.cpp
    src/controllers/CommandRouter.cpp
    src/controllers/PidController.cpp
    src/runtime/ConfigPathResolver.cpp
    src/services/RoadSegmentationService.cpp
    src/services/WebSocketServer.cpp
    src/services/autonomous_control/AutonomousControlDebugRenderer.cpp
    src/services/autonomous_control/AutonomousControlService.cpp
    src/services/autonomous_control/AutonomousControlTelemetry.cpp
    src/services/road_segmentation/RoadSegmentationTelemetry.cpp
    src/services/road_segmentation/VisionRuntimeConfig.cpp
    src/services/traffic_sign_detection/EdgeImpulseTrafficSignDetector.cpp
    src/services/traffic_sign_detection/TrafficSignConfig.cpp
    src/services/traffic_sign_detection/TrafficSignDebugRenderer.cpp
    src/services/traffic_sign_detection/TrafficSignRuntime.cpp
    src/services/traffic_sign_detection/TrafficSignTelemetry.cpp
    src/services/traffic_sign_detection/TrafficSignTemporalFilter.cpp
    src/services/traffic_sign_detection/TrafficSignTypes.cpp
    src/services/vision/VisionDebugStream.cpp
    src/services/vision/VisionDebugViewRenderer.cpp
    src/services/vision/VisionRuntimeTelemetry.cpp
    src/services/vision/VisionSubscriptionRegistry.cpp
    src/services/websocket/ClientRegistry.cpp
    src/services/websocket/WebSocketProtocol.cpp
    src/services/traffic_signals/TrafficSignalRegistry.cpp
)

add_library(autonomous_car_v3_common STATIC
    ${AUTONOMOUS_CAR_V3_COMMON_SOURCES}
)

target_include_directories(autonomous_car_v3_common
    PUBLIC
        src
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(autonomous_car_v3_common
    PUBLIC
        road_segmentation_core
        autonomous_car_v3_traffic_sign_model
        Threads::Threads
        atomic
        ${OpenCV_LIBS}
)

add_executable(autonomous_car_v3_vision_debug
    src/vision_debug_main.cpp
)

target_link_libraries(autonomous_car_v3_vision_debug
    PRIVATE
        autonomous_car_v3_common
)

find_path(WIRINGPI_INCLUDE_DIR wiringPi.h)
find_library(WIRINGPI_LIBRARY wiringPi)

if (WIRINGPI_INCLUDE_DIR AND WIRINGPI_LIBRARY)
    add_executable(${PROJECT_NAME}
        src/main.cpp
        src/controllers/MotorController.cpp
        src/controllers/SteeringController.cpp
        src/commands/forward/ForwardCommand.cpp
        src/commands/backward/BackwardCommand.cpp
        src/commands/stop/StopCommand.cpp
        src/commands/turn_left/TurnLeftCommand.cpp
        src/commands/turn_right/TurnRightCommand.cpp
        src/commands/center_steering/CenterSteeringCommand.cpp
        src/commands/throttle/ThrottleCommand.cpp
        src/commands/steering/SteeringCommand.cpp
    )

    target_include_directories(${PROJECT_NAME}
        PRIVATE
            src
            ${OpenCV_INCLUDE_DIRS}
            ${WIRINGPI_INCLUDE_DIR}
    )

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            autonomous_car_v3_common
            ${WIRINGPI_LIBRARY}
    )
else ()
    message(STATUS "wiringPi nao encontrado; o alvo autonomous_car_v3 sera ignorado. Use autonomous_car_v3_vision_debug para debug local.")
endif ()

enable_testing()

add_executable(autonomous_car_v3_tests
    tests/TestMain.cpp
    tests/AutonomousControlServiceTests.cpp
    tests/AutonomousControlTelemetryTests.cpp
    tests/CommandRouterTests.cpp
    tests/RoadSegmentationTelemetryTests.cpp
    tests/RoadSegmentationServiceTrafficSignIntegrationTests.cpp
    tests/TrafficSignConfigTests.cpp
    tests/TrafficSignDebugRendererTests.cpp
    tests/TrafficSignMailboxTests.cpp
    tests/TrafficSignRuntimeTests.cpp
    tests/TrafficSignTelemetryTests.cpp
    tests/TrafficSignTemporalFilterTests.cpp
    tests/TrafficSignTypesTests.cpp
    tests/VisionDebugViewRendererTests.cpp
    tests/VisionDebugStreamTests.cpp
    tests/VisionRuntimeTelemetryTests.cpp
    tests/VisionRuntimeConfigTests.cpp
    tests/WebSocketProtocolTests.cpp
    tests/WebSocketServerTests.cpp
)

target_link_libraries(autonomous_car_v3_tests
    PRIVATE
        autonomous_car_v3_common
)

add_test(
    NAME autonomous_car_v3_tests
    COMMAND autonomous_car_v3_tests
)
