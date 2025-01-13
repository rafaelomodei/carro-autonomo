#include "TurnCommandHandler.h"
#include <iostream>

void TurnCommandHandler::handle(const rapidjson::Value &cmd) const {
  if (cmd.HasMember("direction") && cmd["direction"].IsString()) {
    std::string direction = cmd["direction"].GetString();
    std::cout << "Comando recebido: Virar para " << direction << std::endl;
  } else {
    std::cerr << "Comando 'turn' inválido: falta 'direction'." << std::endl;
  }
}
