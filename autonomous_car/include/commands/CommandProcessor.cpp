#include "CommandProcessor.h"
#include "handlers/AccelerateCommandHandler.h"
#include "handlers/AlertCommandHandler.h"
#include "handlers/BrakeCommandHandler.h"
#include "handlers/TurnCommandHandler.h"
#include <iostream>
#include <rapidjson/document.h>

CommandProcessor::CommandProcessor() {
  auto driveHandler                = std::make_shared<AccelerateCommandHandler>();
  commandHandlers["accelerate"]    = driveHandler;
  commandHandlers["forward"]       = driveHandler;
  commandHandlers["backward"]      = driveHandler;
  commandHandlers["reverse"]       = driveHandler;
  commandHandlers["stop"]          = driveHandler;
  commandHandlers["brake"]         = std::make_shared<BrakeCommandHandler>();
  commandHandlers["turn"]          = std::make_shared<TurnCommandHandler>();
  commandHandlers["alert"]         = std::make_shared<AlertCommandHandler>();
}

void CommandProcessor::processCommand(const std::string &jsonCommand) {
  auto document = parseJson(jsonCommand);
  if (!document) {
    std::cerr << "Erro ao processar JSON." << std::endl;
    return;
  }

  if (!document->HasMember("type") || !document->HasMember("payload") ||
      !(*document)["type"].IsString()) {
    std::cerr << "Mensagem inválida recebida." << std::endl;
    return;
  }

  std::string messageType = (*document)["type"].GetString();
  const auto &payload     = (*document)["payload"];

  if (messageType == "commands") {
    processPayload(payload);
  } else if (messageType == "config") {
    processConfig(payload);
  } else {
    std::cerr << "Tipo de mensagem desconhecido: " << messageType << std::endl;
  }
}

void CommandProcessor::processConfig(const rapidjson::Value &configPayload) {
  if (!configPayload.IsObject()) {
    std::cerr << "Configuração inválida recebida." << std::endl;
    return;
  }

  auto &vehicleConfig = VehicleConfig::getInstance();
  if (!vehicleConfig.applyUpdate(configPayload)) {
    std::cerr << "Nenhum valor de configuração aplicado." << std::endl;
    return;
  }

  std::cout << "Configuração atualizada: velocidade " << vehicleConfig.speedLimit
            << " | direção " << vehicleConfig.steeringSensitivity
            << " | aceleração " << vehicleConfig.accelerationSensitivity
            << " | freio " << vehicleConfig.brakeSensitivity << std::endl;
}

std::optional<rapidjson::Document> CommandProcessor::parseJson(const std::string &jsonString) const {
  rapidjson::Document doc;
  if (doc.Parse(jsonString.c_str()).HasParseError()) {
    return std::nullopt;
  }
  return doc;
}

void CommandProcessor::processPayload(const rapidjson::Value &payload) {
  if (payload.IsArray()) {
    for (const auto &cmd : payload.GetArray()) {
      processSingleCommand(cmd);
    }
  } else if (payload.IsObject()) {
    processSingleCommand(payload);
  } else {
    std::cerr << "Payload inválido." << std::endl;
  }
}

void CommandProcessor::processSingleCommand(const rapidjson::Value &cmd) {
  if (!cmd.HasMember("action") || !cmd["action"].IsString()) {
    std::cerr << "Comando sem 'action' válida." << std::endl;
    return;
  }

  std::string action = cmd["action"].GetString();
  auto        it     = commandHandlers.find(action);
  if (it != commandHandlers.end()) {
    bool accepted = it->second->handle(cmd);
    if (accepted) {
      std::cout << "Comando aceito: " << action << std::endl;
    } else {
      std::cerr << "Comando rejeitado: " << action << std::endl;
    }
  } else {
    std::cerr << "Comando desconhecido: " << action << std::endl;
  }
}
