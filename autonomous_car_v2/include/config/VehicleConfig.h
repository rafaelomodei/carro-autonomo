#ifndef VEHICLE_CONFIG_H
#define VEHICLE_CONFIG_H

struct PidControl {
  double p = 0.1;
  double i = 0.1;
  double d = 0.1;
};

class VehicleConfig {
public:
  static VehicleConfig &getInstance() {
    static VehicleConfig instance;
    return instance;
  }

  double     speedLimit          = 50.0;
  double     steeringSensitivity = 0.5;
  PidControl pidControl;

  void updateConfig(double speed, double steering, PidControl pid) {
    speedLimit          = speed;
    steeringSensitivity = steering;
    pidControl          = pid;
  }

private:
  VehicleConfig() = default;
};

#endif // VEHICLE_CONFIG_H
