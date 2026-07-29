// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <imgui.h>

namespace sysid {
/**
 * constexpr shim for ImVec2.
 */
struct Vector2d {
  /**
   * X coordinate.
   */
  float x = 0;

  /**
   * Y coordinate.
   */
  float y = 0;

  /**
   * Vector2d addition operator.
   *
   * @param rhs Vector to add.
   * @return Sum of two vectors.
   */
  constexpr Vector2d operator+(const Vector2d& rhs) const {
    return Vector2d{x + rhs.x, y + rhs.y};
  }

  /**
   * Vector2d subtraction operator.
   *
   * @param rhs Vector to subtract.
   * @return Difference of two vectors.
   */
  constexpr Vector2d operator-(const Vector2d& rhs) const {
    return Vector2d{x - rhs.x, y - rhs.y};
  }

  /**
   * Conversion operator to ImVec2.
   */
  explicit operator ImVec2() const { return ImVec2{x, y}; }
};

// App window size (reference; actual display size may differ)
inline constexpr Vector2d kAppWindowSize{1280, 720};

// Menubar height
inline constexpr int kMenubarHeight = 20;

// Gap between window edges
inline constexpr int kWindowGap = 5;

// Left column position and size
inline constexpr Vector2d kLeftColPos{kWindowGap, kMenubarHeight + kWindowGap};
inline constexpr Vector2d kLeftColSize{
    310, kAppWindowSize.y - kLeftColPos.y - kWindowGap};

// Left column contents
inline constexpr Vector2d kLogLoaderWindowPos = kLeftColPos;
inline constexpr Vector2d kLogLoaderWindowSize{kLeftColSize.x, 450};
inline constexpr Vector2d kDataSelectorWindowPos =
    kLogLoaderWindowPos + Vector2d{0, kLogLoaderWindowSize.y + kWindowGap};
inline constexpr Vector2d kDataSelectorWindowSize{
    kLeftColSize.x, kAppWindowSize.y - kWindowGap - kDataSelectorWindowPos.y};

// Center column position and size
inline constexpr Vector2d kCenterColPos =
    kLeftColPos + Vector2d{kLeftColSize.x + kWindowGap, 0};
inline constexpr Vector2d kCenterColSize{
    360, kAppWindowSize.y - kLeftColPos.y - kWindowGap};

// Center column contents
inline constexpr Vector2d kAnalyzerWindowPos = kCenterColPos;
inline constexpr Vector2d kAnalyzerWindowSize{kCenterColSize.x, 550};
inline constexpr Vector2d kProgramLogWindowPos =
    kAnalyzerWindowPos + Vector2d{0, kAnalyzerWindowSize.y + kWindowGap};
inline constexpr Vector2d kProgramLogWindowSize{
    kCenterColSize.x, kAppWindowSize.y - kWindowGap - kProgramLogWindowPos.y};

// Right column position and size
inline constexpr Vector2d kRightColPos =
    kCenterColPos + Vector2d{kCenterColSize.x + kWindowGap, 0};
inline constexpr Vector2d kRightColSize =
    kAppWindowSize - kRightColPos - Vector2d{kWindowGap, kWindowGap};

// Right column contents
inline constexpr Vector2d kDiagnosticPlotWindowPos = kRightColPos;
inline constexpr Vector2d kDiagnosticPlotWindowSize = kRightColSize;

// Text box width as a multiple of the font size
inline constexpr int kTextBoxWidthMultiple = 10;

// ---------------------------------------------------------------------------
// Default GridCell assignments (used by GridLayout on first launch / Reset)
//
// The grid is GridLayout::kGridCols × GridLayout::kGridRows (64 × 35) cells.
// At the reference resolution of 1280×720 each cell is 20×20 px.
// GridLayout scales these assignments to any display size automatically.
//
// Column mapping at 1280×720 (cellW = 20 px):
//   Left   column : col  0 – 15  (cols 0-15,  320 px  ≈ 310 px left col)
//   Center column : col 16 – 33  (cols 16-33, 360 px  ≈ 360 px center col)
//   Right  column : col 34 – 63  (cols 34-63, 600 px  ≈ 590 px right col)
// ---------------------------------------------------------------------------

/// Default GridCell for the "Log Loader" window (left column, top)
inline constexpr int kLogLoaderDefaultCol     = 0;
inline constexpr int kLogLoaderDefaultRow     = 0;
inline constexpr int kLogLoaderDefaultColSpan = 16;
inline constexpr int kLogLoaderDefaultRowSpan = 22;

/// Default GridCell for the "Data Selector" window (left column, bottom)
inline constexpr int kDataSelectorDefaultCol     = 0;
inline constexpr int kDataSelectorDefaultRow     = 22;
inline constexpr int kDataSelectorDefaultColSpan = 16;
inline constexpr int kDataSelectorDefaultRowSpan = 13;

/// Default GridCell for the "Analyzer" window (center column, top)
inline constexpr int kAnalyzerDefaultCol     = 16;
inline constexpr int kAnalyzerDefaultRow     = 0;
inline constexpr int kAnalyzerDefaultColSpan = 18;
inline constexpr int kAnalyzerDefaultRowSpan = 25;

/// Default GridCell for the "Program Log" window (center column, bottom)
inline constexpr int kProgramLogDefaultCol     = 16;
inline constexpr int kProgramLogDefaultRow     = 25;
inline constexpr int kProgramLogDefaultColSpan = 18;
inline constexpr int kProgramLogDefaultRowSpan = 10;

/// Default GridCell for the "Diagnostic Plots" window (right column, full height)
inline constexpr int kDiagnosticPlotsDefaultCol     = 34;
inline constexpr int kDiagnosticPlotsDefaultRow     = 0;
inline constexpr int kDiagnosticPlotsDefaultColSpan = 30;
inline constexpr int kDiagnosticPlotsDefaultRowSpan = 35;

/// Default GridCell for the "Robot Test Runner" window (Full Workspace Safety Focus)
inline constexpr int kRobotRunnerDefaultCol     = 0;
inline constexpr int kRobotRunnerDefaultRow     = 0;
inline constexpr int kRobotRunnerDefaultColSpan = 64;
inline constexpr int kRobotRunnerDefaultRowSpan = 35;

}  // namespace sysid
