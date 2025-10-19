#include "AccelerateCommandHandler.h"

#include "config/VehicleConfig.h"

#include <cmath>
#include <iostream>
#include <pigpio.h>

namespace {
constexpr int PIN_FORWARD_A1  = 17;
constexpr int PIN_FORWARD_A2  = 27;
constexpr int PIN_BACKWARD_B1 = 23;
constexpr int PIN_BACKWARD_B2 = 22;
}

bool AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  std::string action;
  if (cmd.HasMember("action") && cmd["action"].IsString()) {
    action = cmd["action"].GetString();
  }

  bool hasExplicitSpeed = false;
  int  baseSpeed        = 0;

  if (cmd.HasMember("speed") && cmd["speed"].IsNumber()) {
    baseSpeed        = static_cast<int>(std::round(cmd["speed"].GetDouble()));
    hasExplicitSpeed = true;
  } else if (cmd.HasMember("value") && cmd["value"].IsNumber()) {
    baseSpeed        = static_cast<int>(std::round(cmd["value"].GetDouble()));
    hasExplicitSpeed = true;
  }

  if (cmd.HasMember("direction") && cmd["direction"].IsString()) {
    action = cmd["direction"].GetString();
  }

  const auto &vehicleConfig = VehicleConfig::getInstance();

  if (!hasExplicitSpeed) {
    if (action == "forward" || action == "accelerate") {
      baseSpeed = static_cast<int>(std::round(vehicleConfig.speedLimit));
    } else if (action == "backward") {
      baseSpeed = -static_cast<int>(std::round(vehicleConfig.speedLimit));
    } else if (action == "stop") {
      baseSpeed = 0;
    } else if (action == "reverse") {
      baseSpeed = -static_cast<int>(std::round(vehicleConfig.speedLimit));
    }
  }

  if (!hasExplicitSpeed && action.empty()) {
    std::cerr << "Comando de movimento inválido: nenhum parâmetro reconhecido." << std::endl;
    return false;
  }

  const bool goingForward  = baseSpeed > 0;
  const bool goingBackward = baseSpeed < 0;

  double sensitivity = goingForward ? vehicleConfig.accelerationSensitivity
                                    : vehicleConfig.brakeSensitivity;
  int adjustedSpeed  = static_cast<int>(std::round(baseSpeed * sensitivity));

  setMotorDirection(adjustedSpeed);

  if (goingForward) {
    std::cout << "Movimento para frente com sensibilidade " << sensitivity
              << " (valor ajustado: " << adjustedSpeed << ")" << std::endl;
  } else if (goingBackward) {
    std::cout << "Movimento de ré com sensibilidade " << sensitivity
              << " (valor ajustado: " << adjustedSpeed << ")" << std::endl;
  } else {
    std::cout << "Motores em repouso" << std::endl;
  }

  return true;
}

void AccelerateCommandHandler::setupGPIO() const {
  static bool isSetup = false;
  if (!isSetup) {
    gpioSetMode(PIN_FORWARD_A1, PI_OUTPUT);
    gpioSetMode(PIN_FORWARD_A2, PI_OUTPUT);
    gpioSetMode(PIN_BACKWARD_B1, PI_OUTPUT);
    gpioSetMode(PIN_BACKWARD_B2, PI_OUTPUT);
    isSetup = true;
  }
}

void AccelerateCommandHandler::setMotorDirection(int speed) const {
  if (speed > 0) {
    gpioWrite(PIN_FORWARD_A1, PI_HIGH);
    gpioWrite(PIN_BACKWARD_B1, PI_HIGH);
    gpioWrite(PIN_FORWARD_A2, PI_LOW);
    gpioWrite(PIN_BACKWARD_B2, PI_LOW);
  } else if (speed < 0) {
    gpioWrite(PIN_FORWARD_A1, PI_LOW);
    gpioWrite(PIN_BACKWARD_B1, PI_LOW);
    gpioWrite(PIN_FORWARD_A2, PI_HIGH);
    gpioWrite(PIN_BACKWARD_B2, PI_HIGH);
  } else {
    gpioWrite(PIN_FORWARD_A1, PI_LOW);
    gpioWrite(PIN_BACKWARD_B1, PI_LOW);
    gpioWrite(PIN_FORWARD_A2, PI_LOW);
    gpioWrite(PIN_BACKWARD_B2, PI_LOW);
  }
}
