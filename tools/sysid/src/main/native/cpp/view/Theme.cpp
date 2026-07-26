// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/sysid/view/Theme.hpp"

#include <algorithm>
#include <cmath>

#include "wpi/glass/Storage.hpp"

using namespace sysid;

static TeamTheme GetPresetTheme(int index, const float customRGB[3]) {
  TeamTheme theme;
  theme.backgroundColor = ImVec4(0.07f, 0.08f, 0.12f, 1.00f);  // Deep slate background
  theme.cardColor = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);        // Dark card background
  theme.textColor = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);        // Clean white text

  switch (index) {
    case 0:  // Default Cyan
      theme.name = "Default Cyan";
      theme.primaryColor = ImVec4(0.00f, 0.66f, 0.91f, 1.00f);
      theme.secondaryColor = ImVec4(0.00f, 0.52f, 0.75f, 1.00f);
      break;
    case 1:  // FRC Blue
      theme.name = "FRC Blue";
      theme.primaryColor = ImVec4(0.00f, 0.40f, 0.85f, 1.00f);
      theme.secondaryColor = ImVec4(0.00f, 0.30f, 0.68f, 1.00f);
      break;
    case 2:  // FIRST Red
      theme.name = "FIRST Red";
      theme.primaryColor = ImVec4(0.93f, 0.11f, 0.14f, 1.00f);
      theme.secondaryColor = ImVec4(0.72f, 0.08f, 0.10f, 1.00f);
      break;
    case 3:  // FRC Gold
      theme.name = "FRC Gold";
      theme.primaryColor = ImVec4(1.00f, 0.78f, 0.17f, 1.00f);
      theme.secondaryColor = ImVec4(0.85f, 0.62f, 0.10f, 1.00f);
      break;
    case 4:  // FRC Purple
      theme.name = "FRC Purple";
      theme.primaryColor = ImVec4(0.55f, 0.23f, 0.82f, 1.00f);
      theme.secondaryColor = ImVec4(0.42f, 0.16f, 0.65f, 1.00f);
      break;
    case 5:  // Emerald
      theme.name = "Emerald";
      theme.primaryColor = ImVec4(0.06f, 0.73f, 0.51f, 1.00f);
      theme.secondaryColor = ImVec4(0.04f, 0.56f, 0.38f, 1.00f);
      break;
    case 6:  // Cyberpunk
      theme.name = "Cyberpunk";
      theme.primaryColor = ImVec4(0.00f, 0.96f, 0.83f, 1.00f);
      theme.secondaryColor = ImVec4(1.00f, 0.00f, 0.44f, 1.00f);
      break;
    case 7:  // Custom Team Color
    default:
      theme.name = "Custom Team Color";
      theme.primaryColor = ImVec4(customRGB[0], customRGB[1], customRGB[2], 1.00f);
      theme.secondaryColor = ImVec4(
          std::max(0.0f, customRGB[0] - 0.15f),
          std::max(0.0f, customRGB[1] - 0.15f),
          std::max(0.0f, customRGB[2] - 0.15f), 1.00f);
      break;
  }

  return theme;
}

ThemeManager::ThemeManager() {
  UpdateThemeFromPreset();
}

void ThemeManager::UpdateThemeFromPreset() {
  m_currentTheme = GetPresetTheme(m_selectedPreset, m_customRGB.data());
}

void ThemeManager::SelectPreset(int index) {
  m_selectedPreset = std::clamp(index, 0, 7);
  UpdateThemeFromPreset();
  ApplyStyle();
}

void ThemeManager::SetCustomPrimaryColor(float r, float g, float b) {
  m_customRGB[0] = r;
  m_customRGB[1] = g;
  m_customRGB[2] = b;
  if (m_selectedPreset == 7) {
    UpdateThemeFromPreset();
    ApplyStyle();
  }
}

void ThemeManager::ApplyStyle() {
  ImGuiStyle& style = ImGui::GetStyle();

  // Modern Geometry & Spacing
  style.WindowRounding = 8.0f;
  style.ChildRounding = 6.0f;
  style.FrameRounding = 5.0f;
  style.PopupRounding = 8.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 6.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;

  style.WindowPadding = ImVec2(10.0f, 10.0f);
  style.FramePadding = ImVec2(8.0f, 5.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

  // Apply Active Theme Colors
  ImVec4* colors = style.Colors;
  const auto& main = m_currentTheme.primaryColor;
  const auto& sec = m_currentTheme.secondaryColor;
  const auto& bg = m_currentTheme.backgroundColor;
  const auto& card = m_currentTheme.cardColor;
  const auto& text = m_currentTheme.textColor;

  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = ImVec4(text.x * 0.6f, text.y * 0.6f, text.z * 0.6f, 1.00f);

  colors[ImGuiCol_WindowBg] = bg;
  colors[ImGuiCol_ChildBg] = card;
  colors[ImGuiCol_PopupBg] = ImVec4(bg.x * 1.2f, bg.y * 1.2f, bg.z * 1.2f, 0.96f);

  colors[ImGuiCol_Border] = ImVec4(main.x * 0.4f, main.y * 0.4f, main.z * 0.4f, 0.40f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(card.x * 1.3f, card.y * 1.3f, card.z * 1.3f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(main.x * 0.3f, main.y * 0.3f, main.z * 0.3f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(main.x * 0.5f, main.y * 0.5f, main.z * 0.5f, 1.00f);

  colors[ImGuiCol_TitleBg] = ImVec4(card.x * 0.8f, card.y * 0.8f, card.z * 0.8f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(main.x * 0.4f, main.y * 0.4f, main.z * 0.4f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(bg.x, bg.y, bg.z, 0.75f);

  colors[ImGuiCol_MenuBarBg] = ImVec4(card.x * 0.9f, card.y * 0.9f, card.z * 0.9f, 1.00f);

  colors[ImGuiCol_ScrollbarBg] = ImVec4(bg.x, bg.y, bg.z, 0.60f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(main.x * 0.4f, main.y * 0.4f, main.z * 0.4f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = main;
  colors[ImGuiCol_ScrollbarGrabActive] = sec;

  colors[ImGuiCol_CheckMark] = main;
  colors[ImGuiCol_SliderGrab] = main;
  colors[ImGuiCol_SliderGrabActive] = sec;

  colors[ImGuiCol_Button] = ImVec4(main.x * 0.7f, main.y * 0.7f, main.z * 0.7f, 0.85f);
  colors[ImGuiCol_ButtonHovered] = main;
  colors[ImGuiCol_ButtonActive] = sec;

  colors[ImGuiCol_Header] = ImVec4(main.x * 0.4f, main.y * 0.4f, main.z * 0.4f, 0.70f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(main.x * 0.6f, main.y * 0.6f, main.z * 0.6f, 0.80f);
  colors[ImGuiCol_HeaderActive] = main;

  colors[ImGuiCol_Separator] = ImVec4(main.x * 0.4f, main.y * 0.4f, main.z * 0.4f, 0.50f);
  colors[ImGuiCol_SeparatorHovered] = main;
  colors[ImGuiCol_SeparatorActive] = sec;

  colors[ImGuiCol_ResizeGrip] = ImVec4(main.x * 0.3f, main.y * 0.3f, main.z * 0.3f, 0.50f);
  colors[ImGuiCol_ResizeGripHovered] = main;
  colors[ImGuiCol_ResizeGripActive] = sec;

  colors[ImGuiCol_Tab] = ImVec4(card.x * 1.2f, card.y * 1.2f, card.z * 1.2f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(main.x * 0.6f, main.y * 0.6f, main.z * 0.6f, 0.80f);
  colors[ImGuiCol_TabActive] = ImVec4(main.x * 0.5f, main.y * 0.5f, main.z * 0.5f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(card.x, card.y, card.z, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(main.x * 0.3f, main.y * 0.3f, main.z * 0.3f, 1.00f);

  colors[ImGuiCol_PlotLines] = main;
  colors[ImGuiCol_PlotLinesHovered] = sec;
  colors[ImGuiCol_PlotHistogram] = main;
  colors[ImGuiCol_PlotHistogramHovered] = sec;

  colors[ImGuiCol_TableHeaderBg] = ImVec4(card.x * 1.4f, card.y * 1.4f, card.z * 1.4f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(main.x * 0.3f, main.y * 0.3f, main.z * 0.3f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(card.x * 1.5f, card.y * 1.5f, card.z * 1.5f, 0.70f);

  ApplyPlotStyle();
}

void ThemeManager::ApplyPlotStyle() {
  ImPlotStyle& plotStyle = ImPlot::GetStyle();
  plotStyle.PlotPadding = ImVec2(10.0f, 10.0f);
  plotStyle.LabelPadding = ImVec2(5.0f, 5.0f);

  // Synchronize plot theme palette
  const auto& main = m_currentTheme.primaryColor;
  const auto& sec = m_currentTheme.secondaryColor;

  ImVec4 colors[5] = {
      main,                                // Main series (e.g. Filtered)
      ImVec4(1.00f, 0.38f, 0.27f, 1.00f),  // Coral Orange (Raw)
      sec,                                 // Secondary fit
      ImVec4(0.20f, 0.85f, 0.45f, 1.00f),  // Green (Sim)
      ImVec4(0.95f, 0.95f, 0.95f, 1.00f)   // Contrast line
  };
  (void)colors;
}

void ThemeManager::Save(wpi::glass::Storage& storage) {
  storage.SetInt("theme_preset", m_selectedPreset);
  storage.SetDouble("custom_r", m_customRGB[0]);
  storage.SetDouble("custom_g", m_customRGB[1]);
  storage.SetDouble("custom_b", m_customRGB[2]);
}

void ThemeManager::Load(wpi::glass::Storage& storage) {
  m_customRGB[0] = storage.GetDouble("custom_r", 0.0);
  m_customRGB[1] = storage.GetDouble("custom_g", 0.66);
  m_customRGB[2] = storage.GetDouble("custom_b", 1.0);
  // Only restore the preset data here — ApplyStyle() must NOT be called yet
  // because ImGui/ImPlot contexts are not initialized until gui::Initialize().
  // The AddLateExecute callback in App.cpp will apply the style on the first frame.
  m_selectedPreset = std::clamp(storage.GetInt("theme_preset", 0), 0, 7);
  UpdateThemeFromPreset();
}
