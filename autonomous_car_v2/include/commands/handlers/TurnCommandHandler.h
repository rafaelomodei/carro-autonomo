#ifndef TURN_COMMAND_HANDLER_H
#define TURN_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class ServoConfig {
public:
  int initialPulse = 1500; // Pulso inicial (neutro)
  int minPulse     = 1100; // Pulso mínimo para a esquerda
  int maxPulse     = 1850; // Pulso máximo para a direita
  int increment    = 100;   // Sensibilidade (incremento/decremento)
};

class TurnCommandHandler : public CommandHandler {
public:
  TurnCommandHandler();
  void handle(const rapidjson::Value &cmd) const override;

private:
  void setupGPIO() const;
  void setServoPulse(int pulseWidth) const;

  mutable int currentPulse; // Armazena o pulso atual
  ServoConfig config;       // Configurações do servo
};

#endif // TURN_COMMAND_HANDLER_H
