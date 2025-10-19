#ifndef TURN_COMMAND_HANDLER_H
#define TURN_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class TurnCommandHandler : public CommandHandler {
public:
  TurnCommandHandler();
  bool handle(const rapidjson::Value &cmd) const override;

private:
  void setupGPIO() const;
  void setServoPulse(int pulseWidth) const;

  mutable int currentPulse; // Armazena o pulso atual
};

#endif // TURN_COMMAND_HANDLER_H
