#include "BrakeCommandHandler.h"

#include "config/VehicleConfig.h"

#include <cmath>
#include <iostream>

bool BrakeCommandHandler::handle(const rapidjson::Value &cmd) const {
  double intensity = 1.0;
  if (cmd.HasMember("intensity") && cmd["intensity"].IsNumber()) {
    intensity = cmd["intensity"].GetDouble();
  } else if (cmd.HasMember("value") && cmd["value"].IsNumber()) {
    intensity = cmd["value"].GetDouble();
  }

  const auto &vehicleConfig = VehicleConfig::getInstance();
  double      adjusted      = intensity * vehicleConfig.brakeSensitivity;

  std::cout << "Comando de frenagem: intensidade " << intensity
            << " (ajustado: " << adjusted << ")" << std::endl;

  return true;
}
