cmake_minimum_required(VERSION 3.16)
project(autonomous_car_v3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/controllers/MotorController.cpp
    src/controllers/SteeringController.cpp
    src/controllers/CommandDispatcher.cpp
    src/commands/forward/ForwardCommand.cpp
    src/commands/backward/BackwardCommand.cpp
    src/commands/stop/StopCommand.cpp
    src/commands/turn_left/TurnLeftCommand.cpp
    src/commands/turn_right/TurnRightCommand.cpp
    src/commands/center_steering/CenterSteeringCommand.cpp
    src/services/WebSocketServer.cpp
    src/config/ConfigurationManager.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        wiringPi
        Threads::Threads
)
