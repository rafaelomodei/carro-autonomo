#ifndef TURN_COMMAND_HANDLER_H
#define TURN_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class TurnCommandHandler : public CommandHandler {
public:
  void handle(const rapidjson::Value &cmd) const override;
};

#endif // TURN_COMMAND_HANDLER_H
