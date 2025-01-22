cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(AutonomousCar)

# Ativar C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Adicionar os diretórios de include
include_directories(include)

# Adicionar os arquivos de origem
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

# Encontrar a biblioteca Boost
# Encontrar e adicionar Boost
find_package(Boost REQUIRED COMPONENTS system)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(autonomous_car ${Boost_LIBRARIES})
endif()

# Vincular pigpio
find_library(PIGPIO_LIB pigpio REQUIRED)
target_link_libraries(autonomous_car ${PIGPIO_LIB})

# Adicionar a biblioteca pthread (necessária para Boost.Asio)
find_package(Threads REQUIRED)
target_link_libraries(autonomous_car Threads::Threads)

# Adicionar RapidJSON
include_directories(include/libs/rapidjson)

# Encontrar e adicionar OpenCV
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(autonomous_car ${OpenCV_LIBS})
endif()