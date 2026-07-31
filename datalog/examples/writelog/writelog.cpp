#include "wpi/datalog/DataLog.hpp"
#include "wpi/datalog/DataLogBackgroundWriter.hpp"
#include <cmath>

int main() {
  wpi::log::DataLogBackgroundWriter log{"."};
  wpi::log::DoubleLogEntry voltEntry{log, "voltage-Motor-Elevator"};
  wpi::log::DoubleLogEntry tcEntry{log, "torqueCurrent-Motor-Elevator"};
  wpi::log::DoubleLogEntry posEntry{log, "position-Motor-Elevator"};
  wpi::log::DoubleLogEntry velEntry{log, "velocity-Motor-Elevator"};
  wpi::log::StringLogEntry stateEntry{log, "sysid-test-state-Elevator"};

  long long time_us = 1000;
  auto runTest = [&](const char* testName, double currentScale, double velScale) {
    stateEntry.Append(testName, time_us);
    double pos = 0.0;
    for (int i = 0; i < 1000; ++i) { // 2 seconds at 500Hz
      double t = i * 0.002;
      double current = currentScale * t;
      double voltage = current * 0.3; // simulate voltage proportional to current
      double vel = velScale * (1.0 - std::exp(-5.0 * t)); // simulate some dynamics
      pos += vel * 0.002;
      tcEntry.Append(current, time_us);
      voltEntry.Append(voltage, time_us);
      velEntry.Append(vel, time_us);
      posEntry.Append(pos, time_us);
      time_us += 2000; // 2ms step
    }
    stateEntry.Append("none", time_us);
    time_us += 1000000; // wait 1s
  };

  runTest("quasistatic-forward", 1.0, 2.0);
  runTest("quasistatic-reverse", -1.0, -2.0);
  runTest("dynamic-forward", 1.0, 3.0);
  runTest("dynamic-reverse", -1.0, -3.0);

  return 0;
}
