// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "wpi/nt/NetworkTableInstance.hpp"

namespace sysid {

namespace nt = wpi::nt;

struct TelemetrySample {
  double timestamp{0.0};
  double position{0.0};
  double velocity{0.0};
  double voltage{0.0};
  double current{0.0};
};

class RobotConnection {
 public:
  RobotConnection();
  ~RobotConnection();

  // Connection management
  void ConnectTeam(int teamNumber);
  void ConnectIP(std::string_view hostOrIp);
  void Disconnect();

  bool IsConnected() const;
  std::string GetServerAddress() const;
  int GetPingMs() const;

  // Test Execution Controls
  void StartQuasistatic(bool forward, double rampRateVperS);
  void StartDynamic(bool forward, double stepVoltage);
  void EmergencyStop();

  // Safety cutoff limits
  void SetPositionBounds(double minPos, double maxPos, bool enable);
  void SetMaxCurrent(double maxAmps, bool enable);

  // Status & Telemetry
  std::string GetRobotState() const;
  std::vector<TelemetrySample> GetTelemetryHistory() const;
  void ClearTelemetry();

  // Update loop called per frame
  void Update();

 private:
  nt::NetworkTableInstance m_inst;
  std::shared_ptr<nt::NetworkTable> m_table;

  // NT Entries
  nt::NetworkTableEntry m_testTypeEntry;
  nt::NetworkTableEntry m_voltageRampEntry;
  nt::NetworkTableEntry m_stepVoltageEntry;
  nt::NetworkTableEntry m_stateEntry;
  nt::NetworkTableEntry m_telemetryEntry;

  std::string m_serverAddress{"Disconnected"};
  bool m_isConnected{false};

  // Safety limits
  bool m_enablePosLimits{false};
  double m_minPos{0.0};
  double m_maxPos{0.0};

  bool m_enableCurrentLimit{false};
  double m_maxCurrent{40.0};

  // Telemetry buffer
  mutable std::mutex m_telemetryMutex;
  std::vector<TelemetrySample> m_telemetryHistory;
  std::string m_robotState{"Idle"};
};

}  // namespace sysid
