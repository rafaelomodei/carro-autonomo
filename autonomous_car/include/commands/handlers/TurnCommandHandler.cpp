#include "TurnCommandHandler.h"
#include <algorithm> // para std::clamp
#include <cstdlib>
#include <iostream>
#include <pigpio.h>

#define SERVO_PIN 18 // Pino GPIO usado pelo servo
#define MIN_PULSE 1100
#define MAX_PULSE 1750

TurnCommandHandler::TurnCommandHandler()
    : currentPulse(config.initialPulse) { // Inicializa o pulso com o valor inicial
  setupGPIO();
  setServoPulse(currentPulse); // Define o pulso inicial
}

void TurnCommandHandler::handle(const rapidjson::Value &cmd) const {
  if (cmd.HasMember("direction") && cmd["direction"].IsString()) {
    std::string direction   = cmd["direction"].GetString();
    double      sensitivity = VehicleConfig::getInstance().steeringSensitivity; // Obtém a sensibilidade

    int increment = static_cast<int>(config.increment * sensitivity); // Ajusta conforme sensibilidade

    if (direction == "left") {
      currentPulse = std::max(config.minPulse, currentPulse - increment);
    } else if (direction == "right") {
      currentPulse = std::min(config.maxPulse, currentPulse + increment);
    }

    setServoPulse(currentPulse);
  }
}

void TurnCommandHandler::setupGPIO() const {
  static bool isSetup = false;
  if (!isSetup) {
    if (gpioInitialise() < 0) {
      std::cerr << "Falha ao inicializar pigpio." << std::endl;
      exit(1);
    }

    std::cout << "Pino GPIO " << SERVO_PIN << " configurado como saída para o servo." << std::endl;
    isSetup = true;
  }
}

void TurnCommandHandler::setServoPulse(int pulseWidth) const {
  // clamp no intervalo válido antes de chamar o driver
  pulseWidth = std::clamp(pulseWidth, MIN_PULSE, MAX_PULSE);
  gpioServo(SERVO_PIN, pulseWidth);
}