#include "config/VehicleConfig.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
constexpr const char *DEFAULT_CONFIG_PATH = "config/default_config.json";
constexpr const char *ALT_CONFIG_PATH     = "../config/default_config.json";
}

VehicleConfig &VehicleConfig::getInstance() {
  static VehicleConfig instance;
  return instance;
}

VehicleConfig::VehicleConfig() { resetToDefaults(); }

void VehicleConfig::resetToDefaults() {
  speedLimit               = 50.0;
  steeringSensitivity      = 0.5;
  accelerationSensitivity  = 1.0;
  brakeSensitivity         = 1.0;
  pidControl               = {0.1, 0.1, 0.1};
  servoConfig.initialPulse = 1500;
  servoConfig.minPulse     = 1100;
  servoConfig.maxPulse     = 1850;
  servoConfig.increment    = 100;
}

bool VehicleConfig::loadFromFile(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    if (filePath == DEFAULT_CONFIG_PATH) {
      return loadFromFile(ALT_CONFIG_PATH);
    }
    std::cerr << "Não foi possível abrir arquivo de configuração: " << filePath
              << std::endl;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();

  rapidjson::Document doc;
  if (doc.Parse(buffer.str().c_str()).HasParseError()) {
    std::cerr << "Falha ao ler arquivo de configuração: " << filePath
              << std::endl;
    return false;
  }

  if (!doc.IsObject()) {
    std::cerr << "Formato inválido no arquivo de configuração: " << filePath
              << std::endl;
    return false;
  }

  return applyUpdate(doc);
}

bool VehicleConfig::applyUpdate(const rapidjson::Value &configPayload) {
  bool updated = false;

  if (configPayload.HasMember("speedLimit") &&
      configPayload["speedLimit"].IsNumber()) {
    speedLimit = configPayload["speedLimit"].GetDouble();
    updated    = true;
  }

  if (configPayload.HasMember("steeringSensitivity") &&
      configPayload["steeringSensitivity"].IsNumber()) {
    steeringSensitivity = configPayload["steeringSensitivity"].GetDouble();
    updated             = true;
  }

  if (configPayload.HasMember("accelerationSensitivity") &&
      configPayload["accelerationSensitivity"].IsNumber()) {
    accelerationSensitivity =
        configPayload["accelerationSensitivity"].GetDouble();
    updated = true;
  }

  if (configPayload.HasMember("brakeSensitivity") &&
      configPayload["brakeSensitivity"].IsNumber()) {
    brakeSensitivity = configPayload["brakeSensitivity"].GetDouble();
    updated          = true;
  }

  if (configPayload.HasMember("acceleration") &&
      configPayload["acceleration"].IsObject()) {
    const auto &accel = configPayload["acceleration"];
    if (accel.HasMember("sensitivity") && accel["sensitivity"].IsNumber()) {
      accelerationSensitivity = accel["sensitivity"].GetDouble();
      updated                 = true;
    }
  }

  if (configPayload.HasMember("brake") &&
      configPayload["brake"].IsObject()) {
    const auto &brake = configPayload["brake"];
    if (brake.HasMember("sensitivity") && brake["sensitivity"].IsNumber()) {
      brakeSensitivity = brake["sensitivity"].GetDouble();
      updated          = true;
    }
  }

  if (configPayload.HasMember("pidControl") &&
      configPayload["pidControl"].IsObject()) {
    const auto &pid = configPayload["pidControl"];
    if (pid.HasMember("p") && pid["p"].IsNumber()) {
      pidControl.p = pid["p"].GetDouble();
      updated       = true;
    }
    if (pid.HasMember("i") && pid["i"].IsNumber()) {
      pidControl.i = pid["i"].GetDouble();
      updated       = true;
    }
    if (pid.HasMember("d") && pid["d"].IsNumber()) {
      pidControl.d = pid["d"].GetDouble();
      updated       = true;
    }
  }

  if (configPayload.HasMember("servo") &&
      configPayload["servo"].IsObject()) {
    const auto &servo = configPayload["servo"];
    if (servo.HasMember("initialPulse") && servo["initialPulse"].IsInt()) {
      servoConfig.initialPulse = servo["initialPulse"].GetInt();
      updated                  = true;
    }
    if (servo.HasMember("minPulse") && servo["minPulse"].IsInt()) {
      servoConfig.minPulse = servo["minPulse"].GetInt();
      updated              = true;
    }
    if (servo.HasMember("maxPulse") && servo["maxPulse"].IsInt()) {
      servoConfig.maxPulse = servo["maxPulse"].GetInt();
      updated              = true;
    }
    if (servo.HasMember("increment") && servo["increment"].IsInt()) {
      servoConfig.increment = servo["increment"].GetInt();
      updated               = true;
    }
  }

  return updated;
}

void VehicleConfig::updateConfig(double speed, double steering, PidControl pid) {
  speedLimit          = speed;
  steeringSensitivity = steering;
  pidControl          = pid;
}
