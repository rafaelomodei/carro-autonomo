#ifndef ALERT_COMMAND_HANDLER_H
#define ALERT_COMMAND_HANDLER_H

#include "../CommandProcessor.h"

class AlertCommandHandler : public CommandHandler {
public:
  bool handle(const rapidjson::Value &cmd) const override;
};

#endif // ALERT_COMMAND_HANDLER_H
