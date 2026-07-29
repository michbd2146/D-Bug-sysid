// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "wpi/glass/View.hpp"
#include "wpi/sysid/view/RobotConnection.hpp"
#include "wpi/util/Logger.hpp"

namespace wpi::glass {
class Storage;
}  // namespace wpi::glass

namespace sysid {

/**
 * Panel for on-robot characterization routine execution over NetworkTables.
 */
class RobotRunner : public wpi::glass::View {
 public:
  explicit RobotRunner(wpi::glass::Storage& storage, wpi::util::Logger& logger);
  ~RobotRunner() override = default;

  void Display() override;

 private:
  wpi::glass::Storage& m_storage;
  wpi::util::Logger& m_logger;
  RobotConnection m_connection;

  // UI state
  int m_connectionType{0};  // 0: Team Number, 1: Custom IP / Localhost
  int m_teamNumber{2146};
  char m_customIpBuffer[64]{"127.0.0.1"};

  // Control settings
  float m_rampRate{1.0f};
  float m_stepVoltage{7.0f};

  // Safety limits
  bool m_enablePosLimits{false};
  float m_minPos{-10.0f};
  float m_maxPos{10.0f};

  bool m_enableCurrentLimit{true};
  float m_maxCurrent{40.0f};

  // Test execution state
  int m_selectedRoutine{0};  // 0: Quasistatic Fwd, 1: Quasistatic Bwd, 2: Dynamic Fwd, 3: Dynamic Bwd
  bool m_isRunningTest{false};

  void DisplayConnectionSection();
  void DisplayControlsSection();
  void DisplaySafetySection();
  void DisplayTelemetrySection();
};

}  // namespace sysid
