#include "TurnCommandHandler.h"

#include "config/VehicleConfig.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <pigpio.h>

namespace {
constexpr int SERVO_PIN = 18;
}

TurnCommandHandler::TurnCommandHandler()
    : currentPulse(VehicleConfig::getInstance().servoConfig.initialPulse) {
  setupGPIO();
  setServoPulse(currentPulse);
}

bool TurnCommandHandler::handle(const rapidjson::Value &cmd) const {
  if (!cmd.HasMember("direction") || !cmd["direction"].IsString()) {
    std::cerr << "Comando 'turn' inválido: campo 'direction' ausente." << std::endl;
    return false;
  }

  const auto &vehicleConfig = VehicleConfig::getInstance();
  const auto &servoConfig   = vehicleConfig.servoConfig;

  double sensitivity = vehicleConfig.steeringSensitivity;
  int    baseStep    = servoConfig.increment;

  if (cmd.HasMember("value") && cmd["value"].IsNumber()) {
    baseStep = static_cast<int>(std::round(cmd["value"].GetDouble()));
  } else if (cmd.HasMember("degrees") && cmd["degrees"].IsNumber()) {
    baseStep = static_cast<int>(std::round(cmd["degrees"].GetDouble()));
  }

  int increment = static_cast<int>(std::round(baseStep * sensitivity));
  if (increment <= 0) {
    increment = 1;
  }

  const std::string direction = cmd["direction"].GetString();

  if (direction == "left") {
    currentPulse = std::max(servoConfig.minPulse, currentPulse - increment);
  } else if (direction == "right") {
    currentPulse = std::min(servoConfig.maxPulse, currentPulse + increment);
  } else if (direction == "center" || direction == "straight") {
    currentPulse = servoConfig.initialPulse;
  } else {
    std::cerr << "Direção desconhecida: " << direction << std::endl;
    return false;
  }

  setServoPulse(currentPulse);
  std::cout << "Servo ajustado para " << direction << " (pulso " << currentPulse
            << ")" << std::endl;

  return true;
}

void TurnCommandHandler::setupGPIO() const {
  static bool isSetup = false;
  if (!isSetup) {
    gpioSetMode(SERVO_PIN, PI_OUTPUT);
    isSetup = true;
  }
}

void TurnCommandHandler::setServoPulse(int pulseWidth) const {
  const auto &servoConfig = VehicleConfig::getInstance().servoConfig;
  pulseWidth              =
      std::clamp(pulseWidth, servoConfig.minPulse, servoConfig.maxPulse);
  gpioServo(SERVO_PIN, pulseWidth);
}
