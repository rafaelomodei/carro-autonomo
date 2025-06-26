#include "TurnCommandHandler.h"
#include <cstdlib>
#include <iostream>
#include <pigpio.h>
#include <algorithm>    // para std::clamp

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
    std::string direction = cmd["direction"].GetString();
    std::cout << "Comando recebido: " << direction << std::endl;

    if (direction == "left") {
      currentPulse = std::clamp(currentPulse - config.increment,
                                   config.minPulse, config.maxPulse);
      setServoPulse(currentPulse);
      std::cout << "Servo ajustado para " << currentPulse << " µs (esquerda)." << std::endl;
    } else if (direction == "right") {
      currentPulse = std::clamp(currentPulse + config.increment,
                                config.minPulse, config.maxPulse);
      setServoPulse(currentPulse);
      std::cout << "Servo ajustado para " << currentPulse << " µs (direita)." << std::endl;
    } else {
      std::cerr << "Direção inválida: " << direction << std::endl;
    }
  } else {
    std::cerr << "Comando 'turn' inválido: falta 'direction'." << std::endl;
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