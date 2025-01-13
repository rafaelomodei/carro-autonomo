#include "AccelerateCommandHandler.h"
#include <iostream>

void AccelerateCommandHandler::handle(const rapidjson::Value &cmd) const {
  if (cmd.HasMember("speed") && cmd["speed"].IsInt()) {
    int speed = cmd["speed"].GetInt();
    std::cout << "Comando recebido: Acelerar para " << speed << " km/h" << std::endl;
  } else {
    std::cerr << "Comando 'accelerate' inválido: falta 'speed'." << std::endl;
  }
}
