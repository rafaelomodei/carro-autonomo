#include "CommandProcessor.h"
#include "handlers/AccelerateCommandHandler.h"
#include "handlers/AlertCommandHandler.h"
#include "handlers/BrakeCommandHandler.h"
#include "handlers/TurnCommandHandler.h"
#include <iostream>
#include <rapidjson/document.h>

CommandProcessor::CommandProcessor() {
  commandHandlers["accelerate"] = std::make_shared<AccelerateCommandHandler>();
  commandHandlers["brake"]      = std::make_shared<BrakeCommandHandler>();
  commandHandlers["turn"]       = std::make_shared<TurnCommandHandler>();
  commandHandlers["alert"]      = std::make_shared<AlertCommandHandler>();
}

void CommandProcessor::processCommand(const std::string &jsonCommand) {
  auto document = parseJson(jsonCommand);
  if (!document) {
    std::cerr << "Erro ao processar JSON." << std::endl;
    return;
  }

  if (!isCommandMessage(*document)) {
    std::cerr << "Mensagem inválida ou tipo não suportado." << std::endl;
    return;
  }

  processPayload(document->GetObject()["payload"]);
}

void CommandProcessor::processConfig(const rapidjson::Value &configPayload) {
  if (!configPayload.IsObject()) {
    std::cerr << "Configuração inválida recebida." << std::endl;
    return;
  }

  double speedLimit          = configPayload["speedLimit"].GetDouble();
  double steeringSensitivity = configPayload["steeringSensitivity"].GetDouble();

  PidControl pid;
  pid.p = configPayload["pidControl"]["p"].GetDouble();
  pid.i = configPayload["pidControl"]["i"].GetDouble();
  pid.d = configPayload["pidControl"]["d"].GetDouble();

  // Atualiza a configuração global do veículo
  VehicleConfig::getInstance().updateConfig(speedLimit, steeringSensitivity, pid);

  std::cout << "Configuração atualizada: Velocidade " << speedLimit
            << ", Sensibilidade " << steeringSensitivity
            << ", PID(" << pid.p << ", " << pid.i << ", " << pid.d << ")" << std::endl;
}

std::optional<rapidjson::Document> CommandProcessor::parseJson(const std::string &jsonString) const {
  rapidjson::Document doc;
  if (doc.Parse(jsonString.c_str()).HasParseError()) {
    return std::nullopt;
  }
  return doc;
}

bool CommandProcessor::isCommandMessage(const rapidjson::Document &doc) const {
  return doc.HasMember("type") && doc["type"].IsString() && std::string(doc["type"].GetString()) == "commands";
}

void CommandProcessor::processPayload(const rapidjson::Value &payload) {
  if (!payload.IsArray()) {
    std::cerr << "Payload inválido." << std::endl;
    return;
  }

  for (const auto &cmd : payload.GetArray()) {
    processSingleCommand(cmd);
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
    it->second->handle(cmd);
  } else {
    std::cerr << "Comando desconhecido: " << action << std::endl;
  }
}
