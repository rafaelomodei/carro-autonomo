#include "AlertCommandHandler.h"
#include <iostream>

void AlertCommandHandler::handle(const rapidjson::Value &) const {
  std::cout << "Comando recebido: Ligar alerta" << std::endl;
}
