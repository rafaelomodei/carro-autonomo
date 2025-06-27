cmake_minimum_required(VERSION 3.10)
project(AutonomousCarV2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Futuras dependencias podem ser adicionadas aqui
find_package(OpenCV REQUIRED)

# find_library(PIGPIO_LIB pigpio REQUIRED)

include_directories(
    include
    # include/libs/rapidjson
)

add_executable(autonomous_car_v2
    src/main.cpp
    include/managers/WebSocketManager.cpp
    include/video/VideoStreamHandler.cpp
    include/commands/CommandProcessor.cpp
    include/commands/handlers/AccelerateCommandHandler.cpp
    include/commands/handlers/AlertCommandHandler.cpp
    include/commands/handlers/BrakeCommandHandler.cpp
    include/commands/handlers/TurnCommandHandler.cpp
)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_library(PIGPIO_LIB pigpio REQUIRED)

target_link_libraries(autonomous_car_v2
    PRIVATE
        ${Boost_LIBRARIES}
        Threads::Threads
        ${OpenCV_LIBS}
        ${PIGPIO_LIB}
)
