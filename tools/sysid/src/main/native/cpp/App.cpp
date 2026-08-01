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
#include "wpi/sysid/Util.hpp"
#include "wpi/sysid/view/Analyzer.hpp"
#include "wpi/sysid/view/DataSelector.hpp"
#include "wpi/sysid/view/GridLayout.hpp"
#include "wpi/sysid/view/LogLoader.hpp"
#include "wpi/sysid/view/RobotRunner.hpp"
#include "wpi/sysid/view/Theme.hpp"
#include "wpi/sysid/view/UILayout.hpp"
#include "wpi/util/Logger.hpp"
#include "wpi/util/print.hpp"

namespace gui = wpi::gui;

static std::unique_ptr<wpi::glass::WindowManager> gWindowManager;
static sysid::GridLayout gGridLayout;

wpi::glass::Window* gLogLoaderWindow;
wpi::glass::Window* gDataSelectorWindow;
wpi::glass::Window* gAnalyzerWindow;
wpi::glass::Window* gProgramLogWindow;
wpi::glass::Window* gRobotRunnerWindow;
static wpi::glass::MainMenuBar gMainMenu;

// App Mode: 0 = Analyzer Workspace, 1 = Live Robot Runner Safety Mode
static int gAppMode = 0;

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
  auto robotRunner = std::make_unique<sysid::RobotRunner>(storage, gLogger);

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

  gRobotRunnerWindow =
      gWindowManager->AddWindow("Robot Test Runner", std::move(robotRunner));
  gRobotRunnerWindow->SetVisible(false);

  gProgramLogWindow = gWindowManager->AddWindow(
      "Program Log", std::make_unique<wpi::glass::LogView>(&gLog));

  // Register all panels with the grid layout engine.
  gGridLayout.Register("Log Loader",
      sysid::GridCell{sysid::kLogLoaderDefaultCol,
                      sysid::kLogLoaderDefaultRow,
                      sysid::kLogLoaderDefaultColSpan,
                      sysid::kLogLoaderDefaultRowSpan});
  gGridLayout.Register("Data Selector",
      sysid::GridCell{sysid::kDataSelectorDefaultCol,
                      sysid::kDataSelectorDefaultRow,
                      sysid::kDataSelectorDefaultColSpan,
                      sysid::kDataSelectorDefaultRowSpan});
  gGridLayout.Register("Analyzer",
      sysid::GridCell{sysid::kAnalyzerDefaultCol,
                      sysid::kAnalyzerDefaultRow,
                      sysid::kAnalyzerDefaultColSpan,
                      sysid::kAnalyzerDefaultRowSpan});
  gGridLayout.Register("Program Log",
      sysid::GridCell{sysid::kProgramLogDefaultCol,
                      sysid::kProgramLogDefaultRow,
                      sysid::kProgramLogDefaultColSpan,
                      sysid::kProgramLogDefaultRowSpan});
  gGridLayout.Register("Diagnostic Plots",
      sysid::GridCell{sysid::kDiagnosticPlotsDefaultCol,
                      sysid::kDiagnosticPlotsDefaultRow,
                      sysid::kDiagnosticPlotsDefaultColSpan,
                      sysid::kDiagnosticPlotsDefaultRowSpan});

  // Set default positions and sizes for windows upon launch.
  auto ResetLayout = [&storage] {
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
    gRobotRunnerWindow->SetDefaultPos(10.0f, 30.0f);
    gRobotRunnerWindow->SetDefaultSize(1250.0f, 670.0f);
    // Reset the grid layout and persist the defaults
    gGridLayout.Reset();
    gGridLayout.Save(storage);
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

    // ---- Grid layout engine: snap, swap/reject, overlays ------------------
    // GridLayout::Apply() replaces the old hard-coded ClampWindow system.
    // It enforces per-cell positions for non-dragging windows, detects
    // drag-end to snap/swap/reject, draws the preview overlay while dragging,
    // and handles resize-end to update colSpan/rowSpan.
    gGridLayout.Apply();

    // ---- Runtime layout reset (triggered by Widgets → Reset Layout) --------
    // GridLayout::Reset() snaps all cells back to their registered defaults.
    // We persist immediately so the change survives a restart.
    if (gDoResetLayout) {
      gGridLayout.Reset();
      gGridLayout.Save(wpi::glass::GetStorageRoot().GetChild("SysId"));
      gDoResetLayout = false;
    }

    // ---- Enforce locked full-tab layout for Robot Test Runner -------------
    if (gAppMode == 1 && gRobotRunnerWindow) {
      const ImVec2 disp = ImGui::GetIO().DisplaySize;
      gRobotRunnerWindow->SetPos(5.0f, sysid::GridLayout::kMenuBarH + 5.0f, ImGuiCond_Always);
      gRobotRunnerWindow->SetSize(disp.x - 10.0f, disp.y - sysid::GridLayout::kMenuBarH - 10.0f, ImGuiCond_Always);
      gRobotRunnerWindow->SetFlags(ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    }

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
      ImGui::Separator();
      if (ImGui::BeginMenu("Grid Settings")) {
        ImGui::Checkbox("Show grid overlay", &gGridLayout.showGridOverlay);
        sysid::CreateTooltip(
            "Draw faint grid lines over the workspace background so you can "
            "see the snap grid while arranging panels.");
        ImGui::Checkbox("Snap to grid on release", &gGridLayout.snapWhileDragging);
        sysid::CreateTooltip(
            "When enabled, panels snap to the nearest grid cell when you "
            "release the mouse button after dragging. Disable for free movement.");
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }

    bool about = false;
    static bool docs = false;
    if (ImGui::BeginMenu("Info")) {
      if (ImGui::MenuItem("Documentation")) {
        docs = true;
      }
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

    // Mode Selector Tabs (Positioned on the RIGHT side of the control bar)
    float rightOffset = ImGui::GetWindowWidth() - 520.0f;
    if (rightOffset > 400.0f) {
      ImGui::SameLine(rightOffset);
    } else {
      ImGui::SameLine();
    }

    if (ImGui::MenuItem("  Analyzer Workspace  ", nullptr, gAppMode == 0)) {
      gAppMode = 0;
      gLogLoaderWindow->SetVisible(true);
      gDataSelectorWindow->SetVisible(true);
      gAnalyzerWindow->SetVisible(true);
      gProgramLogWindow->SetVisible(true);
      gRobotRunnerWindow->SetVisible(false);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    if (ImGui::MenuItem("  Robot Test Runner (SAFETY MODE)  ", nullptr, gAppMode == 1)) {
      gAppMode = 1;
      gLogLoaderWindow->SetVisible(false);
      gDataSelectorWindow->SetVisible(false);
      gAnalyzerWindow->SetVisible(false);
      gProgramLogWindow->SetVisible(false);
      gRobotRunnerWindow->SetVisible(true);
    }
    ImGui::PopStyleColor();

    ImGui::EndMainMenuBar();

    if (about) {
      ImGui::OpenPopup("About");
      about = false;
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("D-Bug SysId v4.0 - System Identification for Robot Mechanisms");
      ImGui::Separator();
      ImGui::Text("WPILib v%s", GetWPILibVersion());
      gui::EmitRendererInfo();
      ImGui::Separator();
      ImGui::Text("Save location: %s", wpi::glass::GetStorageDir().c_str());
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (docs) {
      ImGui::OpenPopup("SysId Documentation & Theory");
      docs = false;
    }
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("SysId Documentation & Theory", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
      if (ImGui::BeginChild("DocsScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
        ImGui::TextWrapped("The primary purpose of D-Bug SysId is to characterize the physical behavior of robot mechanisms (like elevators, arms, and drivetrains) and calculate optimal control gains for them.");
        
        ImGui::Spacing();
        ImGui::SeparatorText("Mathematical Theory");
        ImGui::BulletText("Feedforward Constants (Ks, Kg, Kv, Ka): Used to predict the necessary voltage to achieve a desired state based on physical modeling.");
        ImGui::Indent();
        ImGui::BulletText("Ks (Static Friction): Voltage required to break static friction.");
        ImGui::BulletText("Kg (Gravity): Voltage required to exactly counteract gravity and hold an elevator or arm still.");
        ImGui::BulletText("Kv (Velocity): Voltage required to maintain a constant velocity of 1 unit/s.");
        ImGui::BulletText("Ka (Acceleration): Voltage required to induce an acceleration of 1 unit/s^2.");
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::BulletText("State-Space Modeling:");
        ImGui::TextWrapped("SysId automatically generates State-Space matrices representing the physical system. This represents a system as a set of first-order differential equations: x_dot = Ax + Bu. These continuous and discrete-time matrices can be used for advanced custom controllers.");
        
        ImGui::Spacing();
        ImGui::BulletText("LQR Tuning (Optimal Control):");
        ImGui::TextWrapped("In the Feedback Analysis section, SysId uses a Linear-Quadratic Regulator to compute the optimal Kp and Kd gains. By balancing the State Cost (Max allowed Error) and Effort Cost (Max Control Voltage), it mathematically guarantees the most efficient PID response to achieve your specified tolerance without exceeding actuator limits.");

        ImGui::Spacing();
        ImGui::SeparatorText("How to Use the Tool");
        ImGui::BulletText("1. Load Data: Open D-Bug SysId and drag and drop your .wpilog file into the UI.");
        ImGui::BulletText("2. Analyze Feedforward: Select the mechanism type and units. SysId graphs the velocity/acceleration responses and calculates Ks, Kg, Kv, and Ka.");
        ImGui::BulletText("3. Analyze Feedback: Scroll down to the Optimal Control (LQR) section. Adjust your Error and Effort constraints until you find a balance that suits your mechanism.");
        ImGui::BulletText("4. Export Code: Select your target framework (e.g. CTRE Phoenix 6) and copy the generated snippet. The snippet includes properly scaled gains ready to paste directly into your robot code.");
      }
      ImGui::EndChild();
      
      ImGui::Separator();
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    // Welcome / landing page modal
    if (gShowWelcome) {
      ImGui::OpenPopup("Welcome to D-Bug SysId v4.0");
    }
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Welcome to D-Bug SysId v4.0",
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

  gui::Initialize("D-Bug SysId v4.0", sysid::kAppWindowSize.x,
                  sysid::kAppWindowSize.y, gui::RendererPreference::PREFER_2D);
  gui::Main();

  wpi::glass::DestroyContext();
  gui::DestroyContext();
}

#endif
