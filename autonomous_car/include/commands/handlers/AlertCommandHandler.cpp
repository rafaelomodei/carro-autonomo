#include "AlertCommandHandler.h"
#include <iostream>

bool AlertCommandHandler::handle(const rapidjson::Value &) const {
  std::cout << "Comando recebido: Ligar alerta" << std::endl;
  return true;
}
