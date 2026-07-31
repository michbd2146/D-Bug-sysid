#include "wpi/datalog/DataLog.hpp"
#include "wpi/datalog/DataLogBackgroundWriter.hpp"
#include <cmath>

int main() {
  wpi::log::DataLogBackgroundWriter log{"."};
  wpi::log::DoubleLogEntry tcEntry{log, "torqueCurrent-Motor-Elevator"};
  wpi::log::DoubleLogEntry voltEntry{log, "voltage-Motor-Elevator"};
  wpi::log::DoubleLogEntry posEntry{log, "position-Motor-Elevator"};
  wpi::log::DoubleLogEntry velEntry{log, "velocity-Motor-Elevator"};
  wpi::log::StringLogEntry stateEntry{log, "sysid-test-state-Elevator"};

  long long time_us = 1000;
  
  double Kg = 5.0; // Amps (Gravity)
  double Kv = 1.5; // Amps / (m/s)
  double Ka = 0.2; // Amps / (m/s^2)
  double Ks = 0.5; // Amps (Static Friction)

  auto runTest = [&](const char* testName, bool isDynamic, bool isForward) {
    stateEntry.Append(testName, time_us);
    
    double pos = 0.0;
    double vel = 0.0;
    
    double rampRate = 10.0; // 10 A/s
    double stepCurrent = 40.0; // 40 A

    for (int i = 0; i < 2000; ++i) { // 4 seconds at 500Hz
      double t = i * 0.002;
      double current = 0.0;
      
      if (isDynamic) {
        current = isForward ? stepCurrent : -stepCurrent;
      } else {
        current = isForward ? (rampRate * t) : -(rampRate * t);
      }
      
      // I = Kg + Ks*sgn(v) + Kv*v + Ka*a 
      // a = (I - Kg - Ks*sgn(v) - Kv*v) / Ka
      
      // Prevent movement until current overcomes gravity + friction
      double accel = 0.0;
      double netForce = current - Kg;
      
      if (vel > 0.01) {
        accel = (netForce - Ks - Kv * vel) / Ka;
      } else if (vel < -0.01) {
        accel = (netForce + Ks - Kv * vel) / Ka;
      } else {
        // Stationary
        if (netForce > Ks) {
          accel = (netForce - Ks) / Ka;
        } else if (netForce < -Ks) {
          accel = (netForce + Ks) / Ka;
        } else {
          accel = 0.0;
        }
      }
      
      vel += accel * 0.002;
      pos += vel * 0.002;
      
      tcEntry.Append(current, time_us);
      voltEntry.Append(current * 0.1, time_us); // fake voltage just in case
      velEntry.Append(vel, time_us);
      posEntry.Append(pos, time_us);
      
      time_us += 2000;
    }
    stateEntry.Append("none", time_us);
    time_us += 1000000;
  };

  runTest("quasistatic-forward", false, true);
  runTest("quasistatic-reverse", false, false);
  runTest("dynamic-forward", true, true);
  runTest("dynamic-reverse", true, false);

  return 0;
}
