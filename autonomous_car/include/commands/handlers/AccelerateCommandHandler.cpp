#include "AccelerateCommandHandler.h"
#include <iostream>
#include <pigpio.h>

// Definições dos pinos GPIO
constexpr int PIN_FORWARD_A1  = 17; // GPIO para direção Forward
constexpr int PIN_FORWARD_A2  = 27; // GPIO para direção Forward
constexpr int PIN_BACKWARD_B1 = 23; // GPIO para direção Backward
constexpr int PIN_BACKWARD_B2 = 22; // GPIO para direção Backward

void AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  if (cmd.HasMember("speed") && cmd["speed"].IsInt()) {
    int speed = cmd["speed"].GetInt();
    std::cout << "Comando recebido: Acelerar para " << speed << " km/h" << std::endl;

    setMotorDirection(speed);

    // Converter velocidade para PWM (0-255)
    int pwmValue = abs(speed * 255 / 100);  // Assumindo 100 km/h como velocidade máxima
    pwmValue     = std::min(pwmValue, 255); // Garantir que não ultrapasse o máximo permitido
  } else {
    std::cerr << "Comando 'accelerate' inválido: falta 'speed'." << std::endl;
  }
}

void AccelerateCommandHandler::setupGPIO() const {
  static bool isSetup = false;
  if (!isSetup) {
    if (gpioInitialise() < 0) {
      std::cerr << "Falha ao inicializar pigpio." << std::endl;
      exit(1);
    }
    gpioSetMode(PIN_FORWARD_A1, PI_OUTPUT);  // Configura pino Forward como saída
    gpioSetMode(PIN_FORWARD_A2, PI_OUTPUT); // Configura pino Backward como saída
    gpioSetMode(PIN_BACKWARD_B1, PI_OUTPUT);  // Configura pino Forward como saída
    gpioSetMode(PIN_BACKWARD_B2, PI_OUTPUT); // Configura pino Backward como saída
    isSetup = true;
  }
}

void AccelerateCommandHandler::setMotorDirection(int speed) const {
  if (speed > 0) {
      std::cerr << "FRENTE." << std::endl;

    gpioWrite(PIN_FORWARD_A1, PI_HIGH); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B1, PI_HIGH); // Desativa direção Backward
    gpioWrite(PIN_FORWARD_A2, PI_LOW); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B2, PI_LOW); // Ativa direção Forward
  } else if (speed < 0) {
      std::cerr << "DANDO RE." << std::endl;

    gpioWrite(PIN_FORWARD_A1, PI_LOW); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B1, PI_LOW); // Desativa direção Backward
    gpioWrite(PIN_FORWARD_A2, PI_HIGH); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B2, PI_HIGH); // Ativa direção Forward
    
  } else {
      std::cerr << "PARADO." << std::endl;

    gpioWrite(PIN_FORWARD_A1, PI_LOW); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B1, PI_LOW); // Desativa direção Backward
    gpioWrite(PIN_FORWARD_A2, PI_LOW); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD_B2, PI_LOW); // Ativa direção Forward
  }
}

