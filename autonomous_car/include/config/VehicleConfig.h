#ifndef VEHICLE_CONFIG_H
#define VEHICLE_CONFIG_H

#include <rapidjson/document.h>
#include <string>

struct PidControl {
  double p = 0.1;
  double i = 0.1;
  double d = 0.1;
};

struct ServoConfig {
  int initialPulse = 1500;
  int minPulse     = 1100;
  int maxPulse     = 1850;
  int increment    = 100;
};

class VehicleConfig {
public:
  static VehicleConfig &getInstance();

  bool loadFromFile(const std::string &filePath);
  bool applyUpdate(const rapidjson::Value &configPayload);
  void resetToDefaults();

  double     speedLimit          = 50.0;
  double     steeringSensitivity = 0.5;
  double     accelerationSensitivity = 1.0;
  double     brakeSensitivity        = 1.0;
  PidControl pidControl;
  ServoConfig servoConfig;

  void updateConfig(double speed, double steering, PidControl pid);

private:
  VehicleConfig();
};

#endif // VEHICLE_CONFIG_H
