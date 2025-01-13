#include "BrakeCommandHandler.h"
#include <iostream>

void BrakeCommandHandler::handle(const rapidjson::Value &) const {
  std::cout << "Comando recebido: Frear" << std::endl;
}
