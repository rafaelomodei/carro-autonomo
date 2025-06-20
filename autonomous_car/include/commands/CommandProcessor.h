#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "../config/VehicleConfig.h"
#include <functional>
#include <memory>
#include <optional>
#include <rapidjson/document.h>
#include <string>
#include <unordered_map>

// Interface para Handlers
class CommandHandler {
public:
  virtual ~CommandHandler()                              = default;
  virtual void handle(const rapidjson::Value &cmd) const = 0;
};

// Processador de Comandos
class CommandProcessor {
public:
  CommandProcessor();
  void processCommand(const std::string &command);

private:
  std::unordered_map<std::string, std::shared_ptr<CommandHandler>> commandHandlers;

  // Funções auxiliares
  std::optional<rapidjson::Document> parseJson(const std::string &jsonString) const;
  bool                               isCommandMessage(const rapidjson::Document &doc) const;
  void                               processPayload(const rapidjson::Value &payload);
  void                               processSingleCommand(const rapidjson::Value &cmd);
  void                               processConfig(const rapidjson::Value &configPayload);
};

#endif // COMMAND_PROCESSOR_H
