// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include <algorithm>
#include <cstdio>

#ifndef RUNNING_SYSID_TESTS

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <imgui.h>
#include <imgui_internal.h>

#include "wpi/gui/wpigui_internal.hpp"

#include "wpi/glass/Context.hpp"
#include "wpi/glass/MainMenuBar.hpp"
#include "wpi/glass/Storage.hpp"
#include "wpi/glass/Window.hpp"
#include "wpi/glass/WindowManager.hpp"
#include "wpi/glass/other/Log.hpp"
#include "wpi/datalog/DataLogReaderThread.hpp"
#include "wpi/gui/wpigui.hpp"
#include "wpi/gui/wpigui_openurl.hpp"
#include "wpi/sysid/view/Analyzer.hpp"
#include "wpi/sysid/view/DataSelector.hpp"
#include "wpi/sysid/view/LogLoader.hpp"
#include "wpi/sysid/view/Theme.hpp"
#include "wpi/sysid/view/UILayout.hpp"
#include "wpi/util/Logger.hpp"
#include "wpi/util/print.hpp"

namespace gui = wpi::gui;

static std::unique_ptr<wpi::glass::WindowManager> gWindowManager;

wpi::glass::Window* gLogLoaderWindow;
wpi::glass::Window* gDataSelectorWindow;
wpi::glass::Window* gAnalyzerWindow;
wpi::glass::Window* gProgramLogWindow;
static wpi::glass::MainMenuBar gMainMenu;

wpi::glass::LogData gLog;
wpi::util::Logger gLogger;

// Welcome modal state
static bool gShowWelcome = true;
static bool gWelcomeDontShowAgain = false;

// Set to true by the Reset Layout menu item; consumed in AddLateExecute
// to force window positions with ImGuiCond_Always (SetDefaultPos uses
// ImGuiCond_FirstUseEver which only fires once).
// Initialized to true so every launch always starts at default positions,
// ignoring any stale positions saved in the .ini file.
static bool gDoResetLayout = true;

const char* GetWPILibVersion();

namespace sysid {
std::string_view GetResource_sysid_16_png();
std::string_view GetResource_sysid_32_png();
std::string_view GetResource_sysid_48_png();
std::string_view GetResource_sysid_64_png();
std::string_view GetResource_sysid_128_png();
std::string_view GetResource_sysid_256_png();
std::string_view GetResource_sysid_512_png();
}  // namespace sysid

void Application(std::string_view saveDir) {
  // Create the wpigui (along with Dear ImGui) and Glass contexts.
  gui::CreateContext();
  wpi::glass::CreateContext();

  // Add icons
  gui::AddIcon(sysid::GetResource_sysid_16_png());
  gui::AddIcon(sysid::GetResource_sysid_32_png());
  gui::AddIcon(sysid::GetResource_sysid_48_png());
  gui::AddIcon(sysid::GetResource_sysid_64_png());
  gui::AddIcon(sysid::GetResource_sysid_128_png());
  gui::AddIcon(sysid::GetResource_sysid_256_png());
  gui::AddIcon(sysid::GetResource_sysid_512_png());

  wpi::glass::SetStorageName("sysid");
  wpi::glass::SetStorageDir(saveDir.empty() ? gui::GetPlatformSaveFileDir()
                                            : saveDir);

  // Use a crisp, modern font and match the clear color to the dark theme
  // so there's no grey flash on startup. This runs after ImGui is initialized.
  gui::AddInit([] {
    auto* ctx = gui::GetCurrentContext();
    if (ctx) {
      ctx->defaultFontName = "Roboto Regular";
    }
    gui::SetClearColor(ImVec4(0.08f, 0.09f, 0.12f, 1.0f));
  });

  // Also set font name before Initialize() so the first-run default is correct.
  if (auto* ctx = gui::GetCurrentContext()) {
    ctx->defaultFontName = "Roboto Regular";
  }


  // Add messages from the global sysid logger into the Log window.
  gLogger.SetLogger([](unsigned int level, const char* file, unsigned int line,
                       const char* msg) {
    const char* lvl = "";
    if (level >= wpi::util::WPI_LOG_CRITICAL) {
      lvl = "CRITICAL: ";
    } else if (level >= wpi::util::WPI_LOG_ERROR) {
      lvl = "ERROR: ";
    } else if (level >= wpi::util::WPI_LOG_WARNING) {
      lvl = "WARNING: ";
    } else if (level >= wpi::util::WPI_LOG_INFO) {
      lvl = "INFO: ";
    } else if (level >= wpi::util::WPI_LOG_DEBUG) {
      lvl = "DEBUG: ";
    }
    std::string filename = std::filesystem::path{file}.filename().string();
    gLog.Append(std::format("{}{} ({}:{})\n", lvl, msg, filename, line));
#ifndef NDEBUG
    wpi::util::print(stderr, "{}{} ({}:{})\n", lvl, msg, filename, line);
#endif
  });

  gLogger.set_min_level(wpi::util::WPI_LOG_DEBUG);

  // Initialize window manager and add views.
  auto& storage = wpi::glass::GetStorageRoot().GetChild("SysId");
  gWindowManager = std::make_unique<wpi::glass::WindowManager>(storage);
  gWindowManager->GlobalInit();

  static sysid::ThemeManager gThemeManager;
  gThemeManager.Load(storage);

  auto logLoader = std::make_unique<sysid::LogLoader>(storage, gLogger);
  auto dataSelector = std::make_unique<sysid::DataSelector>(storage, gLogger);
  auto analyzer = std::make_unique<sysid::Analyzer>(storage, gLogger);

  logLoader->unload.connect([ds = dataSelector.get()] { ds->Reset(); });
  // Connect the logLoader's load signal to dataSelector's SetReader method so that
  // when a file completes reading, DataSelector automatically scans entries for SysId routines.
  logLoader->load.connect(
      [ds = dataSelector.get()](wpi::log::DataLogReaderThread* reader) {
        ds->SetReader(reader);
      });
  dataSelector->testdata = [_analyzer = analyzer.get(),
                            ds = dataSelector.get()](auto data) {
    _analyzer->m_data = data;
    _analyzer->SetMissingTests(ds->m_missingTests);
    _analyzer->AnalyzeData();
  };

  gLogLoaderWindow =
      gWindowManager->AddWindow("Log Loader", std::move(logLoader));

  gDataSelectorWindow =
      gWindowManager->AddWindow("Data Selector", std::move(dataSelector));

  gAnalyzerWindow = gWindowManager->AddWindow("Analyzer", std::move(analyzer));

  gProgramLogWindow = gWindowManager->AddWindow(
      "Program Log", std::make_unique<wpi::glass::LogView>(&gLog));

  // Set default positions and sizes for windows, and lock them so they
  // cannot be dragged on top of each other.
  auto ResetLayout = [] {
    gLogLoaderWindow->SetDefaultPos(sysid::kLogLoaderWindowPos.x,
                                    sysid::kLogLoaderWindowPos.y);
    gLogLoaderWindow->SetDefaultSize(sysid::kLogLoaderWindowSize.x,
                                     sysid::kLogLoaderWindowSize.y);
    gDataSelectorWindow->SetDefaultPos(sysid::kDataSelectorWindowPos.x,
                                       sysid::kDataSelectorWindowPos.y);
    gDataSelectorWindow->SetDefaultSize(sysid::kDataSelectorWindowSize.x,
                                        sysid::kDataSelectorWindowSize.y);
    gAnalyzerWindow->SetDefaultPos(sysid::kAnalyzerWindowPos.x,
                                   sysid::kAnalyzerWindowPos.y);
    gAnalyzerWindow->SetDefaultSize(sysid::kAnalyzerWindowSize.x,
                                    sysid::kAnalyzerWindowSize.y);
    gProgramLogWindow->SetDefaultPos(sysid::kProgramLogWindowPos.x,
                                     sysid::kProgramLogWindowPos.y);
    gProgramLogWindow->SetDefaultSize(sysid::kProgramLogWindowSize.x,
                                      sysid::kProgramLogWindowSize.y);
  };
  ResetLayout();
  gProgramLogWindow->DisableRenamePopup();

  // Windows are freely movable but column-clamped each frame (see
  // AddLateExecute below) so they cannot overlap across columns.

  // Configure save file.
  gui::ConfigurePlatformSaveFile("sysid.ini");

  // Add menu bar and apply theme style inside the active ImGui frame loop.
  gui::AddLateExecute([] {
    gThemeManager.ApplyStyle();

    // ---- Runtime layout reset (triggered by Widgets → Reset Layout) ----
    // Uses ImGuiCond_Always so it overrides the saved .ini position.
    if (gDoResetLayout) {
      ImGui::SetWindowPos("Log Loader",
          ImVec2(sysid::kLogLoaderWindowPos.x, sysid::kLogLoaderWindowPos.y),
          ImGuiCond_Always);
      ImGui::SetWindowSize("Log Loader",
          ImVec2(sysid::kLogLoaderWindowSize.x, sysid::kLogLoaderWindowSize.y),
          ImGuiCond_Always);
      ImGui::SetWindowPos("Data Selector",
          ImVec2(sysid::kDataSelectorWindowPos.x, sysid::kDataSelectorWindowPos.y),
          ImGuiCond_Always);
      ImGui::SetWindowSize("Data Selector",
          ImVec2(sysid::kDataSelectorWindowSize.x, sysid::kDataSelectorWindowSize.y),
          ImGuiCond_Always);
      ImGui::SetWindowPos("Analyzer",
          ImVec2(sysid::kAnalyzerWindowPos.x, sysid::kAnalyzerWindowPos.y),
          ImGuiCond_Always);
      ImGui::SetWindowSize("Analyzer",
          ImVec2(sysid::kAnalyzerWindowSize.x, sysid::kAnalyzerWindowSize.y),
          ImGuiCond_Always);
      ImGui::SetWindowPos("Program Log",
          ImVec2(sysid::kProgramLogWindowPos.x, sysid::kProgramLogWindowPos.y),
          ImGuiCond_Always);
      ImGui::SetWindowSize("Program Log",
          ImVec2(sysid::kProgramLogWindowSize.x, sysid::kProgramLogWindowSize.y),
          ImGuiCond_Always);
      ImGui::SetWindowPos("Diagnostic Plots",
          ImVec2(sysid::kDiagnosticPlotWindowPos.x, sysid::kDiagnosticPlotWindowPos.y),
          ImGuiCond_Always);
      ImGui::SetWindowSize("Diagnostic Plots",
          ImVec2(sysid::kDiagnosticPlotWindowSize.x, sysid::kDiagnosticPlotWindowSize.y),
          ImGuiCond_Always);
      gDoResetLayout = false;
    }

    // ---- Per-frame column clamping ----
    // Clamp each panel's X position so it cannot cross into another column.
    // This lets users rearrange within their column but prevents overlap
    // between the left (log/data), center (analyzer/log), and right (plots)
    // columns. Y is clamped to stay on screen.
    auto ClampWindow = [](const char* name, float minX, float maxX,
                          float minY, float maxY) {
      ImGuiWindow* win = ImGui::FindWindowByName(name);
      if (!win || win->Hidden) return;
      ImVec2 pos = win->Pos;
      ImVec2 sz  = win->Size;
      // Guard against a window wider/taller than its allowed range.
      // If the window doesn't fit, pin it to the min edge instead of crashing.
      float clampedX = (maxX - sz.x >= minX)
                           ? std::clamp(pos.x, minX, maxX - sz.x)
                           : minX;
      float clampedY = (maxY - sz.y >= minY)
                           ? std::clamp(pos.y, minY, maxY - sz.y)
                           : minY;
      if (clampedX != pos.x || clampedY != pos.y) {
        ImGui::SetWindowPos(name, ImVec2(clampedX, clampedY), ImGuiCond_Always);
      }
    };
    const float leftMax  = sysid::kLeftColPos.x + sysid::kLeftColSize.x;
    const float centMin  = sysid::kCenterColPos.x;
    const float centMax  = sysid::kCenterColPos.x + sysid::kCenterColSize.x;
    const float rightMin = sysid::kRightColPos.x;
    const float appH     = sysid::kAppWindowSize.y;
    const float menuH    = static_cast<float>(sysid::kMenubarHeight);
    ClampWindow("Log Loader",       sysid::kWindowGap, leftMax,  menuH, appH);
    ClampWindow("Data Selector",    sysid::kWindowGap, leftMax,  menuH, appH);
    ClampWindow("Analyzer",         centMin, centMax,            menuH, appH);
    ClampWindow("Program Log",      centMin, centMax,            menuH, appH);
    ClampWindow("Diagnostic Plots", rightMin,
                sysid::kAppWindowSize.x - sysid::kWindowGap,     menuH, appH);

    ImGui::BeginMainMenuBar();
    gMainMenu.WorkspaceMenu();
    gui::EmitViewMenu();

    if (ImGui::BeginMenu("Team Theme")) {
      for (int i = 0; i < 8; ++i) {
        bool selected = (gThemeManager.GetSelectedPresetIndex() == i);
        if (ImGui::MenuItem(sysid::ThemeManager::kPresetNames[i], nullptr, selected)) {
          gThemeManager.SelectPreset(i);
          gThemeManager.Save(wpi::glass::GetStorageRoot().GetChild("SysId"));
        }
      }

      if (gThemeManager.GetSelectedPresetIndex() == 7) {
        ImGui::Separator();
        if (ImGui::ColorEdit3("Custom Primary", gThemeManager.GetCustomColorArray())) {
          gThemeManager.SelectPreset(7);
          gThemeManager.Save(wpi::glass::GetStorageRoot().GetChild("SysId"));
        }
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Widgets")) {
      gWindowManager->DisplayMenu();
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Layout")) {
        gDoResetLayout = true;
      }
      ImGui::EndMenu();
    }

    bool about = false;
    if (ImGui::BeginMenu("Info")) {
      if (ImGui::MenuItem("About")) {
        about = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("Welcome Guide")) {
        gShowWelcome = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Online documentation")) {
        wpi::gui::OpenURL(
            "https://docs.wpilib.org/en/stable/docs/software/pathplanning/"
            "system-identification/");
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    if (about) {
      ImGui::OpenPopup("About");
      about = false;
    }
    if (ImGui::BeginPopupModal("About")) {
      ImGui::Text("SysId: System Identification for Robot Mechanisms");
      ImGui::Separator();
      ImGui::Text("v%s", GetWPILibVersion());
      gui::EmitRendererInfo();
      ImGui::Separator();
      ImGui::Text("Save location: %s", wpi::glass::GetStorageDir().c_str());
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    // Welcome / landing page modal
    if (gShowWelcome) {
      ImGui::OpenPopup("Welcome to D-Bug SysId");
    }
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Welcome to D-Bug SysId",
                               nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove)) {
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                         "How to characterize your robot mechanism:");
      ImGui::Spacing();

      // Step 1
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Step 1");
      ImGui::SameLine(); ImGui::Text("Load a .wpilog file");
      ImGui::TextWrapped(
          "  Use the Log Loader panel (top-left) to open a log recorded "
          "from your robot while running the SysId routine.");
      ImGui::Spacing();

      // Step 2
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Step 2");
      ImGui::SameLine(); ImGui::Text("Select your test data");
      ImGui::TextWrapped(
          "  In the Data Selector panel (bottom-left), pick the SysId "
          "routine and select the four test types: quasistatic forward/"
          "backward and dynamic forward/backward.");
      ImGui::Spacing();

      // Step 3
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Step 3");
      ImGui::SameLine(); ImGui::Text("Review the calculated gains");
      ImGui::TextWrapped(
          "  The Analyzer panel (center) automatically computes Ks, Kv, "
          "Ka, Kg and feedback gains Kp/Kd. The System Dynamics card "
          "shows bandwidth and settling time.");
      ImGui::Spacing();

      // Step 4
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Step 4");
      ImGui::SameLine(); ImGui::Text("Copy gains to your robot code");
      ImGui::TextWrapped(
          "  Click \"Copy as Code\" to copy ready-to-paste Java, C++, "
          "CTRE Phoenix 6, and REV SPARK snippets directly to the clipboard.");
      ImGui::Spacing();

      ImGui::Separator();
      ImGui::TextDisabled(
          "Tip: No log yet? Enter Kv & Ka manually in the Analyzer "
          "to preview theoretical gains and system dynamics.");
      ImGui::Spacing();

      ImGui::Checkbox("Don't show this again", &gWelcomeDontShowAgain);
      ImGui::SameLine();
      float btnWidth = 80.0f;
      ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - btnWidth);
      if (ImGui::Button("Got it!", ImVec2(btnWidth, 0))) {
        gShowWelcome = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
    if (!gShowWelcome) {
      // Already dismissed — only re-open if explicitly requested via menu
      gShowWelcome = false;
    }
  });

  gui::Initialize("System Identification", sysid::kAppWindowSize.x,
                  sysid::kAppWindowSize.y, gui::RendererPreference::PREFER_2D);
  gui::Main();

  wpi::glass::DestroyContext();
  gui::DestroyContext();
}

#endif
