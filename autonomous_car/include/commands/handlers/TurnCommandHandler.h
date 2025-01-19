#ifndef TURN_COMMAND_HANDLER_H
#define TURN_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class TurnCommandHandler : public CommandHandler {
public:
  void handle(const rapidjson::Value &cmd) const override;

private:
  void setupGPIO() const;
  void setServoPulse(int pulseWidth) const; // Define o pulso diretamente em µs

  mutable int currentPulse = 1500; // Pulso inicial (neutro, 1500 µs)
};

#endif // TURN_COMMAND_HANDLER_H
