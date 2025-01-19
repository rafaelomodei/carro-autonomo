#include "TurnCommandHandler.h"
#include <pigpio.h>
#include <iostream>
#include <cstdlib>

#define SERVO_PIN 18      // Pino GPIO usado pelo servo
#define MIN_PULSE 500    // Pulso mínimo para o servo (~1000 µs)
#define MAX_PULSE 2500    // Pulso máximo para o servo (~2000 µs)

void TurnCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  if (cmd.HasMember("direction") && cmd["direction"].IsString()) {
    std::string direction = cmd["direction"].GetString();
    std::cout << "Comando recebido: " << direction << std::endl;

    if (direction == "left") {
      setServoPulse(1000); // Pulso fixo para a esquerda
      std::cout << "Servo ajustado para 1000 µs (esquerda)." << std::endl;
    } else if (direction == "right") {
      setServoPulse(2000); // Pulso fixo para a direita
      std::cout << "Servo ajustado para 2000 µs (direita)." << std::endl;
    } else if (direction == "test" && cmd.HasMember("value") && cmd["value"].IsInt()) {
      int pulseWidth = cmd["value"].GetInt();
      if (pulseWidth >= MIN_PULSE && pulseWidth <= MAX_PULSE) {
        setServoPulse(pulseWidth); // Define o pulso recebido no comando
        std::cout << "Servo ajustado para " << pulseWidth << " µs (teste)." << std::endl;
      } else {
        std::cerr << "Valor inválido para o pulso: " << pulseWidth << " µs." << std::endl;
      }
    } else {
      std::cerr << "Direção ou valor inválido no comando." << std::endl;
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



