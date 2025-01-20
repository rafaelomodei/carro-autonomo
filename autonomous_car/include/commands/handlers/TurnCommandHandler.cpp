#include "TurnCommandHandler.h"
#include <cstdlib>
#include <iostream>
#include <pigpio.h>

#define SERVO_PIN 18 // Pino GPIO usado pelo servo

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
      currentPulse = std::max(config.minPulse, currentPulse - config.increment); // Decrementa até o limite mínimo
      setServoPulse(currentPulse);
      std::cout << "Servo ajustado para " << currentPulse << " µs (esquerda)." << std::endl;
    } else if (direction == "right") {
      currentPulse = std::min(config.maxPulse, currentPulse + config.increment); // Incrementa até o limite máximo
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
  gpioServo(SERVO_PIN, pulseWidth); // Define a largura de pulso no servo
}
