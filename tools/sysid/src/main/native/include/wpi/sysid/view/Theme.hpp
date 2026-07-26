// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <array>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

namespace wpi::glass {
class Storage;
}  // namespace wpi::glass

namespace sysid {

/**
 * Represents a color scheme for the SysId application UI.
 */
struct TeamTheme {
  std::string name;
  ImVec4 primaryColor;     // Primary accent (active buttons, headers, highlights)
  ImVec4 secondaryColor;   // Secondary accent (hovered elements, active tabs)
  ImVec4 backgroundColor;  // Window background
  ImVec4 cardColor;        // Child container / card background
  ImVec4 textColor;        // Primary text
};

/**
 * Manages modern ImGui styling and team-customizable visual themes for SysId.
 */
class ThemeManager {
 public:
  static constexpr const char* kPresetNames[] = {
      "Default Cyan", "FRC Blue", "FIRST Red", "FRC Gold",
      "FRC Purple",   "Emerald",  "Cyberpunk", "Custom Team Color"};

  ThemeManager();

  /**
   * Applies the modern ImGui styling rules and active theme colors.
   */
  void ApplyStyle();

  /**
   * Selects a preset theme by index.
   */
  void SelectPreset(int index);

  /**
   * Updates the custom team primary accent color (RGB 0.0 - 1.0).
   */
  void SetCustomPrimaryColor(float r, float g, float b);

  /**
   * Returns the current theme configuration.
   */
  const TeamTheme& GetCurrentTheme() const { return m_currentTheme; }

  /**
   * Returns the index of the currently selected preset.
   */
  int GetSelectedPresetIndex() const { return m_selectedPreset; }

  /**
   * Returns a mutable pointer to the custom primary RGB array for ImGui ColorEdit3.
   */
  float* GetCustomColorArray() { return m_customRGB.data(); }

  /**
   * Saves the active theme preferences to glass storage.
   */
  void Save(wpi::glass::Storage& storage);

  /**
   * Loads saved theme preferences from glass storage.
   */
  void Load(wpi::glass::Storage& storage);

  /**
   * Synchronizes ImPlot styling and color palette to match the active theme.
   */
  void ApplyPlotStyle();

 private:
  int m_selectedPreset = 0;
  TeamTheme m_currentTheme;
  std::array<float, 3> m_customRGB{0.0f, 0.66f, 1.0f};

  void UpdateThemeFromPreset();
};

}  // namespace sysid
