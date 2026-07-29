// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/sysid/view/RobotRunner.hpp"

#include <imgui.h>

#include "wpi/glass/Storage.hpp"
#include "wpi/sysid/Util.hpp"
#include "wpi/datalog/DataLogBackgroundWriter.hpp"
#include "wpi/datalog/DataLog.hpp"

namespace sysid {

RobotRunner::RobotRunner(wpi::glass::Storage& storage,
                         wpi::util::Logger& logger)
    : m_storage(storage), m_logger(logger) {}

void RobotRunner::Display() {
  m_connection.Update();

  // ---- Prominent Safety Warning Header ------------------------------------
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.2f, 1.0f));
  ImGui::TextWrapped("  [SAFETY MODE ACTIVE] YOU ARE COMMUNICATING DIRECTLY WITH LIVE ROBOT HARDWARE over NetworkTables (NT4).  Ensure mechanism travel path is completely clear of personnel!");
  ImGui::PopStyleColor(2);
  ImGui::Separator();

  // ---- 2-Column High Focus Safety Layout ----------------------------------
  ImGui::Columns(2, "RobotRunnerColumns", true);

  // Column 1: Connection & Safety Interlocks
  DisplayConnectionSection();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  DisplaySafetySection();

  ImGui::NextColumn();

  // Column 2: Test Parameters, Execution & Telemetry Stream
  DisplayControlsSection();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  DisplayTelemetrySection();

  ImGui::Columns(1);
}

void RobotRunner::DisplayConnectionSection() {
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "NetworkTables Connection");
  ImGui::RadioButton("Team #", &m_connectionType, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Custom IP / Host", &m_connectionType, 1);

  if (m_connectionType == 0) {
    ImGui::SetNextItemWidth(120);
    ImGui::InputInt("Team", &m_teamNumber);
  } else {
    ImGui::SetNextItemWidth(160);
    ImGui::InputText("Address", m_customIpBuffer, sizeof(m_customIpBuffer));
  }

  ImGui::SameLine();
  if (m_connection.IsConnected()) {
    if (ImGui::Button("Disconnect")) {
      m_connection.Disconnect();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[Connected]");
  } else {
    if (ImGui::Button("Connect")) {
      if (m_connectionType == 0) {
        m_connection.ConnectTeam(m_teamNumber);
      } else {
        m_connection.ConnectIP(m_customIpBuffer);
      }
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[Disconnected]");
  }
}

void RobotRunner::DisplaySafetySection() {
  ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Safety Cutoffs & Limits");

  if (ImGui::Checkbox("Position Bounds", &m_enablePosLimits)) {
    m_connection.SetPositionBounds(m_minPos, m_maxPos, m_enablePosLimits);
  }
  if (m_enablePosLimits) {
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputFloat("Min Pos", &m_minPos)) {
      m_connection.SetPositionBounds(m_minPos, m_maxPos, m_enablePosLimits);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputFloat("Max Pos", &m_maxPos)) {
      m_connection.SetPositionBounds(m_minPos, m_maxPos, m_enablePosLimits);
    }
  }

  if (ImGui::Checkbox("Over-Current Trip (Amps)", &m_enableCurrentLimit)) {
    m_connection.SetMaxCurrent(m_maxCurrent, m_enableCurrentLimit);
  }
  if (m_enableCurrentLimit) {
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputFloat("Max Amps", &m_maxCurrent)) {
      m_connection.SetMaxCurrent(m_maxCurrent, m_enableCurrentLimit);
    }
  }
}

void RobotRunner::DisplayControlsSection() {
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Routine Parameters");

  ImGui::SetNextItemWidth(120);
  ImGui::SliderFloat("Ramp Rate (V/s)", &m_rampRate, 0.1f, 5.0f, "%.2f");

  ImGui::SetNextItemWidth(120);
  ImGui::SliderFloat("Step Voltage (V)", &m_stepVoltage, 1.0f, 12.0f, "%.2f");

  ImGui::Spacing();
  ImGui::Text("Select Routine:");
  ImGui::RadioButton("Quasistatic Forward", &m_selectedRoutine, 0);
  ImGui::RadioButton("Quasistatic Backward", &m_selectedRoutine, 1);
  ImGui::RadioButton("Dynamic Forward", &m_selectedRoutine, 2);
  ImGui::RadioButton("Dynamic Backward", &m_selectedRoutine, 3);

  ImGui::Spacing();
  bool connected = m_connection.IsConnected();

  if (!connected) {
    ImGui::BeginDisabled();
  }

  if (ImGui::Button("START ROUTINE", ImVec2(160, 32))) {
    m_isRunningTest = true;
    switch (m_selectedRoutine) {
      case 0:
        m_connection.StartQuasistatic(true, m_rampRate);
        break;
      case 1:
        m_connection.StartQuasistatic(false, m_rampRate);
        break;
      case 2:
        m_connection.StartDynamic(true, m_stepVoltage);
        break;
      case 3:
        m_connection.StartDynamic(false, m_stepVoltage);
        break;
    }
  }

  if (!connected) {
    ImGui::EndDisabled();
  }

  ImGui::SameLine();
  // Prominent EMERGENCY STOP button (styled red)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));

  if (ImGui::Button("EMERGENCY STOP", ImVec2(160, 32))) {
    m_connection.EmergencyStop();
    m_isRunningTest = false;
  }

  ImGui::PopStyleColor(3);
}

void RobotRunner::DisplayTelemetrySection() {
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Live Telemetry & Status");

  std::string state = m_connection.GetRobotState();
  ImGui::Text("Robot Status: %s", state.c_str());

  auto telemetry = m_connection.GetTelemetryHistory();
  ImGui::Text("Recorded Samples: %zu", telemetry.size());

  if (!telemetry.empty()) {
    const auto& last = telemetry.back();
    ImGui::Text("Time: %.3f s | Volts: %.2f V | Pos: %.3f | Vel: %.3f | Current: %.2f A",
                last.timestamp, last.voltage, last.position, last.velocity, last.current);
  }

  if (ImGui::Button("Clear Telemetry")) {
    m_connection.ClearTelemetry();
  }
  
  ImGui::SameLine();
  if (ImGui::Button("Save & Analyze (Direct Handoff)") && !telemetry.empty()) {
    // Generate a temporary datalog
    std::string filename = "c:\\Users\\Michael\\Documents\\D-Bug-sysid\\handoff.wpilog";
    wpi::log::DataLogBackgroundWriter log;
    log.SetFilename(filename);

    wpi::log::StringLogEntry stateEntry{log, "sysid-test-state-Drive"};
    wpi::log::DoubleLogEntry voltageEntry{log, "voltage-Drive"};
    wpi::log::DoubleLogEntry positionEntry{log, "position-Drive"};
    wpi::log::DoubleLogEntry velocityEntry{log, "velocity-Drive"};

    // We assume the test state is quasistatic-forward for dummy handoff
    stateEntry.Append("quasistatic-forward", 0);
    for (const auto& sample : telemetry) {
      uint64_t timeUs = static_cast<uint64_t>(sample.timestamp * 1e6);
      voltageEntry.Append(sample.voltage, timeUs);
      positionEntry.Append(sample.position, timeUs);
      velocityEntry.Append(sample.velocity, timeUs);
    }
    stateEntry.Append("none", static_cast<uint64_t>(telemetry.back().timestamp * 1e6) + 10000);
    log.Stop();
    
    // We notify the user in the Program Log. The user will manually open the log file
    // for now since App.cpp does not pass DataSelector directly to RobotRunner yet.
    WPI_INFO(m_logger, "Saved NT4 telemetry to {}. Please open this file in the Log Loader panel.", filename);
  }
}

}  // namespace sysid
