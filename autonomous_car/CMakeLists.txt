cmake_minimum_required(VERSION 3.10)
project(AutonomousCar)

# ───── Configuração global ─────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)   # otimizado + debug

# ---------- chave full / lite ----------
option(ENABLE_EI_INFERENCE "Build with Edge Impulse inference" ON)

# ---------- macros de porting ----------
add_compile_definitions(
    EI_PORTING_POSIX=1
    EI_CLASSIFIER_TFLITE_ENABLE=1
    EI_CLASSIFIER_IMAGE_ENABLED=1
)

# ---------- ccache (se disponível) -----
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message(STATUS "ccache encontrado: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# ---------- includes do projeto --------
include_directories(
    include
    include/libs/rapidjson
)

# ---------- fontes do usuário ----------
add_executable(autonomous_car
    src/main.cpp
    include/managers/WebSocketManager.cpp
    include/commands/CommandProcessor.cpp
    include/commands/handlers/AccelerateCommandHandler.cpp
    include/commands/handlers/AlertCommandHandler.cpp
    include/commands/handlers/BrakeCommandHandler.cpp
    include/commands/handlers/TurnCommandHandler.cpp
    include/video/VideoStreamHandler.cpp
)

# ---------- Edge Impulse opcional -------
set(EXTRA_LIBS "")
if (ENABLE_EI_INFERENCE)
    message(STATUS "Edge Impulse: ON")
    file(GLOB_RECURSE EDGE_IMPULSE_SRC CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/include/edge-impulse-sdk/*.c*
        ${PROJECT_SOURCE_DIR}/include/model-parameters/*.cpp
        ${PROJECT_SOURCE_DIR}/include/tflite-model/*.c*
    )
    add_library(edge_impulse STATIC ${EDGE_IMPULSE_SRC})
    target_include_directories(edge_impulse PUBLIC include)
    target_link_libraries(edge_impulse PUBLIC Threads::Threads m)
    list(APPEND EXTRA_LIBS edge_impulse)
else()
    message(STATUS "Edge Impulse: OFF  (lite build)")
    add_compile_definitions(EI_DISABLED)
endif()

# ---------- dependências externas -------
find_package(Boost   REQUIRED COMPONENTS system)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)
find_library(PIGPIO_LIB  pigpio        REQUIRED)
find_library(CODEC2_LIB  codec2        REQUIRED)
find_library(KISSFFT_LIB NAMES kissfft-float kissfft REQUIRED)

# ---------- link final ------------------
target_link_libraries(autonomous_car
    PRIVATE
        ${EXTRA_LIBS}
        ${OpenCV_LIBS}
        ${CODEC2_LIB}
        ${KISSFFT_LIB}
        ${Boost_LIBRARIES}
        ${PIGPIO_LIB}
        Threads::Threads
)

# ---------- PCH opcional ----------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_precompile_headers(autonomous_car
        PRIVATE
            <opencv2/opencv.hpp>
            <edge-impulse-sdk/classifier/ei_run_classifier.h>
    )
endif()
