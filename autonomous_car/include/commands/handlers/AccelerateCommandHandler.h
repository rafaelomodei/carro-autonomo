#ifndef ACCELERATE_COMMAND_HANDLER_H
#define ACCELERATE_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class AccelerateCommandHandler : public CommandHandler {
public:
  void handle(const rapidjson::Value &cmd) const override;
};

#endif // ACCELERATE_COMMAND_HANDLER_H
