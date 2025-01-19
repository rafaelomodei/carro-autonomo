#include "AccelerateCommandHandler.h"
#include <iostream>
#include <pigpio.h>

void AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  if (cmd.HasMember("speed") && cmd["speed"].IsInt()) {
    int speed = cmd["speed"].GetInt();
    std::cout << "Comando recebido: Acelerar para " << speed << " km/h" << std::endl;

    // Converter velocidade para PWM (0-255)
    int pwmValue = (speed * 255) / 100; // Assumindo 100 km/h como velocidade máxima
    gpioPWM(18, pwmValue);              // Configura PWM no pino 18
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
    gpioSetMode(18, PI_OUTPUT); // Configura o pino 18 como saída
    isSetup = true;
  }
}
