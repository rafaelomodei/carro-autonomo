#ifndef BRAKE_COMMAND_HANDLER_H
#define BRAKE_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class BrakeCommandHandler : public CommandHandler {
public:
  bool handle(const rapidjson::Value &cmd) const override;
};

#endif // BRAKE_COMMAND_HANDLER_H
