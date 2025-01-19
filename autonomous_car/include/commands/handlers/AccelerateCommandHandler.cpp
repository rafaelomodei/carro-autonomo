#include "AccelerateCommandHandler.h"
#include <iostream>
#include <pigpio.h>

// Definições dos pinos GPIO
constexpr int PIN_FORWARD  = 23; // GPIO para direção Forward
constexpr int PIN_BACKWARD = 24; // GPIO para direção Backward
constexpr int PIN_PWM      = 18; // GPIO para controle de velocidade (PWM)

void AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  setupGPIO();

  if (cmd.HasMember("speed") && cmd["speed"].IsInt()) {
    int speed = cmd["speed"].GetInt();
    std::cout << "Comando recebido: Acelerar para " << speed << " km/h" << std::endl;

    setMotorDirection(speed);

    // Converter velocidade para PWM (0-255)
    int pwmValue = abs(speed * 255 / 100);  // Assumindo 100 km/h como velocidade máxima
    pwmValue     = std::min(pwmValue, 255); // Garantir que não ultrapasse o máximo permitido
    gpioPWM(PIN_PWM, pwmValue);             // Configura o PWM para o pino de velocidade
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
    gpioSetMode(PIN_FORWARD, PI_OUTPUT);  // Configura pino Forward como saída
    gpioSetMode(PIN_BACKWARD, PI_OUTPUT); // Configura pino Backward como saída
    gpioSetMode(PIN_PWM, PI_OUTPUT);      // Configura pino PWM como saída
    gpioPWM(PIN_PWM, 0);                  // Inicializa PWM com 0% de duty cycle
    isSetup = true;
  }
}

void AccelerateCommandHandler::setMotorDirection(int speed) const {
  if (speed > 0) {
    gpioWrite(PIN_FORWARD, PI_HIGH); // Ativa direção Forward
    gpioWrite(PIN_BACKWARD, PI_LOW); // Desativa direção Backward
  } else if (speed < 0) {
    gpioWrite(PIN_FORWARD, PI_LOW);   // Desativa direção Forward
    gpioWrite(PIN_BACKWARD, PI_HIGH); // Ativa direção Backward
  } else {
    gpioWrite(PIN_FORWARD, PI_LOW); // Desativa ambas as direções
    gpioWrite(PIN_BACKWARD, PI_LOW);
  }
}
