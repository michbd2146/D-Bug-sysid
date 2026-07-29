// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/sysid/view/RobotConnection.hpp"

#include <format>

namespace sysid {

RobotConnection::RobotConnection()
    : m_inst(nt::NetworkTableInstance::Create()) {
  m_table = m_inst.GetTable("SysIdRun");

  // Create entries
  m_testTypeEntry = m_table->GetEntry("sysid-test-type");
  m_voltageRampEntry = m_table->GetEntry("sysid-voltage-ramp");
  m_stepVoltageEntry = m_table->GetEntry("sysid-step-voltage");
  m_stateEntry = m_table->GetEntry("sysid-state");
  m_telemetryEntry = m_table->GetEntry("sysid-telemetry");

  m_testTypeEntry.SetString("none");
  m_voltageRampEntry.SetDouble(1.0);
  m_stepVoltageEntry.SetDouble(7.0);
}

RobotConnection::~RobotConnection() {
  m_inst.StopClient();
  nt::NetworkTableInstance::Destroy(m_inst);
}

void RobotConnection::ConnectTeam(int teamNumber) {
  m_inst.StopClient();
  m_inst.StartClient("D-Bug SysId");
  m_inst.SetServerTeam(teamNumber);
  m_serverAddress = std::format("Team {}", teamNumber);
}

void RobotConnection::ConnectIP(std::string_view hostOrIp) {
  m_inst.StopClient();
  m_inst.StartClient("D-Bug SysId");
  m_inst.SetServer(std::string(hostOrIp).c_str());
  m_serverAddress = std::string(hostOrIp);
}

void RobotConnection::Disconnect() {
  EmergencyStop();
  m_inst.StopClient();
  m_serverAddress = "Disconnected";
  m_isConnected = false;
}

bool RobotConnection::IsConnected() const {
  return m_inst.IsConnected();
}

std::string RobotConnection::GetServerAddress() const {
  return m_serverAddress;
}

int RobotConnection::GetPingMs() const {
  if (!IsConnected()) {
    return 0;
  }
  return static_cast<int>(m_inst.GetNetworkMode());
}

void RobotConnection::StartQuasistatic(bool forward, double rampRateVperS) {
  if (!IsConnected()) {
    return;
  }
  m_voltageRampEntry.SetDouble(rampRateVperS);
  if (forward) {
    m_testTypeEntry.SetString("quasistatic-forward");
  } else {
    m_testTypeEntry.SetString("quasistatic-backward");
  }
}

void RobotConnection::StartDynamic(bool forward, double stepVoltage) {
  if (!IsConnected()) {
    return;
  }
  m_stepVoltageEntry.SetDouble(stepVoltage);
  if (forward) {
    m_testTypeEntry.SetString("dynamic-forward");
  } else {
    m_testTypeEntry.SetString("dynamic-backward");
  }
}

void RobotConnection::EmergencyStop() {
  m_testTypeEntry.SetString("none");
  m_voltageRampEntry.SetDouble(0.0);
  m_stepVoltageEntry.SetDouble(0.0);
}

void RobotConnection::SetPositionBounds(double minPos, double maxPos, bool enable) {
  m_minPos = minPos;
  m_maxPos = maxPos;
  m_enablePosLimits = enable;
}

void RobotConnection::SetMaxCurrent(double maxAmps, bool enable) {
  m_maxCurrent = maxAmps;
  m_enableCurrentLimit = enable;
}

std::string RobotConnection::GetRobotState() const {
  return m_robotState;
}

std::vector<TelemetrySample> RobotConnection::GetTelemetryHistory() const {
  std::lock_guard<std::mutex> lock(m_telemetryMutex);
  return m_telemetryHistory;
}

void RobotConnection::ClearTelemetry() {
  std::lock_guard<std::mutex> lock(m_telemetryMutex);
  m_telemetryHistory.clear();
}

void RobotConnection::Update() {
  m_isConnected = m_inst.IsConnected();
  if (!m_isConnected) {
    m_robotState = "Disconnected";
    return;
  }

  // Update robot state string
  m_robotState = m_stateEntry.GetString("Idle");

  // Read telemetry array updates from NT
  std::vector<double> arr = m_telemetryEntry.GetDoubleArray({});
  if (arr.size() >= 4) {
    TelemetrySample sample;
    sample.timestamp = arr[0];
    sample.voltage = arr[1];
    sample.position = arr[2];
    sample.velocity = arr[3];
    if (arr.size() >= 5) {
      sample.current = arr[4];
    }

    // Check safety bounds during active run
    if (m_enablePosLimits) {
      if (sample.position < m_minPos || sample.position > m_maxPos) {
        EmergencyStop();
        m_robotState = "E-STOP: Position Limit Exceeded!";
      }
    }

    if (m_enableCurrentLimit) {
      if (sample.current > m_maxCurrent) {
        EmergencyStop();
        m_robotState = "E-STOP: Current Trip Exceeded!";
      }
    }

    std::lock_guard<std::mutex> lock(m_telemetryMutex);
    m_telemetryHistory.push_back(sample);
    if (m_telemetryHistory.size() > 2000) {
      m_telemetryHistory.erase(m_telemetryHistory.begin());
    }
  }
}

}  // namespace sysid
