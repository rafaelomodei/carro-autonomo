#include "AccelerateCommandHandler.h"
#include <iostream>
#include <wiringPi.h>

void AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  if (cmd.HasMember("speed") && cmd["speed"].IsInt()) {
    int speed = cmd["speed"].GetInt();
    std::cout << "Comando recebido: Acelerar para " << speed << " km/h" << std::endl;

    // Mapeamento da velocidade para PWM (exemplo)
    int pwmValue = (speed * 1023) / 100; // Assumindo que 100 km/h é o máximo
    pwmWrite(18, pwmValue);              // Pino GPIO 18 configurado como saída PWM
  } else {
    std::cerr << "Comando 'accelerate' inválido: falta 'speed'." << std::endl;
  }
}

void AccelerateCommandHandler::setupGPIO() const {
  static bool isSetup = false;
  if (!isSetup) {
    wiringPiSetupGpio();     // Configura o GPIO com numeração BCM
    pinMode(18, PWM_OUTPUT); // Define o pino 18 como saída PWM
    isSetup = true;
  }
}
