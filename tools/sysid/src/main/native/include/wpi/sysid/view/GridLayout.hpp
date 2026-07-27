// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <string>
#include <vector>

#include <imgui.h>

namespace wpi::glass {
class Storage;
}  // namespace wpi::glass

namespace sysid {

// ---------------------------------------------------------------------------
// GridCell – logical grid occupancy descriptor for one panel
// ---------------------------------------------------------------------------
/**
 * Describes the position and span of a window on the logical grid.
 *
 * The grid is kGridCols × kGridRows cells.  Cell (0,0) is the top-left corner
 * of the workspace (just below the menu bar).  Pixel sizes are computed each
 * frame from the actual display size so the layout scales on any monitor.
 */
struct GridCell {
  int col     = 0;  ///< Left-most column (0-based)
  int row     = 0;  ///< Top-most row     (0-based)
  int colSpan = 1;  ///< Width  in columns
  int rowSpan = 1;  ///< Height in rows
};

// ---------------------------------------------------------------------------
// GridLayout – per-frame grid-snapping layout engine
// ---------------------------------------------------------------------------
/**
 * GridLayout manages a set of named ImGui windows on a logical grid.
 *
 * ## Design decisions
 *
 * ### Dynamic cell sizing
 * Cell pixel dimensions are computed each frame from `ImGui::GetIO().DisplaySize`
 * so the layout adapts to any screen resolution automatically.
 *
 * ### Drag-and-snap
 * While the user drags a window ImGui controls the position freely (smooth
 * drag).  On release GridLayout detects the drag ended and snaps the window to
 * the nearest grid cell.
 *
 * ### Collision / swap policy
 * Before committing a new cell assignment GridLayout checks whether another
 * registered window already occupies the target cell:
 *  - If no collision → move accepted.
 *  - If exactly one other window occupies the target and its old cell (the
 *    dragged window's origin) is free for it → **swap** the two windows.
 *  - If swap would cascade into further collisions → **reject** the move and
 *    snap the dragged window back to its original cell, drawing a red reject
 *    overlay for kRejectFlashSec seconds.
 *
 * ### Drag preview overlay
 * While a window is being dragged a translucent overlay is drawn at the
 * snapped target cell:
 *  - **Green** when the move/swap is valid.
 *  - **Red**   when the move would be rejected.
 *
 * ### Persistence
 * `Save()` / `Load()` store each window's col/row/colSpan/rowSpan in the
 * `wpi::glass::Storage` root, which is written to `sysid.ini`.
 */
class GridLayout {
 public:
  // ---- Grid topology -------------------------------------------------------

  /// Number of logical columns the workspace is divided into.
  static constexpr int kGridCols = 64;
  /// Number of logical rows the workspace is divided into (below menu bar).
  static constexpr int kGridRows = 35;
  /// Height of the OS menu bar in pixels (used to offset the grid origin).
  static constexpr float kMenuBarH = 20.0f;
  /// How long the red reject flash overlay stays visible (seconds).
  static constexpr double kRejectFlashSec = 0.6;

  // ---- Public API ----------------------------------------------------------

  /**
   * Register a named ImGui window with a default grid cell.
   *
   * @param name        The exact ImGui window name string.
   * @param defaultCell The cell to use on first run or after Reset().
   */
  void Register(const char* name, GridCell defaultCell);

  /**
   * Apply the grid layout for this frame.
   *
   * Call once per frame from AddLateExecute, before ImGui::EndMainMenuBar().
   * This method:
   *  1. Computes the current cell pixel sizes from the display size.
   *  2. Detects drag start / end for each registered window.
   *  3. On drag-end: resolves snapping, collision, and swap/reject.
   *  4. Enforces pixel positions for non-dragging windows.
   *  5. Detects resize completion and updates colSpan/rowSpan.
   *  6. Draws the drag-preview overlay and the optional grid overlay.
   */
  void Apply();

  /**
   * Reset all windows to their registered default cells.
   * Does NOT call Save(); the caller is responsible for persisting afterwards.
   */
  void Reset();

  /**
   * Persist cell assignments into glass Storage.
   * @param storage  The glass storage root used for this application.
   */
  void Save(wpi::glass::Storage& storage);

  /**
   * Load cell assignments from glass Storage.
   * Unknown or missing entries fall back to the registered defaults.
   * @param storage  The glass storage root used for this application.
   */
  void Load(wpi::glass::Storage& storage);

  // ---- Display toggles (driven by menu checkboxes in App.cpp) --------------

  bool showGridOverlay  = false;  ///< Draw faint grid lines each frame
  bool snapWhileDragging = true;  ///< Snap-to-grid on drag release (toggle off = free movement)

 private:
  // ---- Per-window state ----------------------------------------------------

  struct WindowEntry {
    std::string name;        ///< ImGui window name
    GridCell    defaultCell; ///< Factory default
    GridCell    cell;        ///< Current cell assignment

    // Drag tracking
    bool  wasDragging  = false; ///< Was the window being dragged last frame?
    GridCell dragOrigin;        ///< Cell at drag start (for reject snap-back)

    // Size & resize tracking
    ImVec2 lastSize{0, 0};
    bool   wasResizing = false;

    // Explicit reset flag to force ImGui position and size update
    bool forceReset = false;
  };

  // ---- Helpers -------------------------------------------------------------

  /// Compute pixel size of one grid cell for the current display size.
  /// X = displayWidth / kGridCols, Y = (displayHeight - kMenuBarH) / kGridRows
  static ImVec2 CellSize();

  /// Convert a grid cell to its top-left pixel position.
  static ImVec2 CellToPixel(const GridCell& c);

  /// Convert a pixel position to the nearest grid cell (clamps to valid range).
  static GridCell PixelToCell(ImVec2 px);

  /// Compute nearest top-left snapped GridCell for a window at windowPos with specified span.
  static GridCell GetSnappedCell(ImVec2 windowPos, int colSpan, int rowSpan);

  /// Return true if any registered entry OTHER than 'skip' occupies 'c'.
  bool IsCellOccupied(const GridCell& c, const WindowEntry* skip) const;

  /// Find the entry that occupies cell 'c', or nullptr.
  WindowEntry* FindOccupant(const GridCell& c, const WindowEntry* skip);

  /// Draw the preview rectangle for a drag operation.
  void DrawDragPreview(const GridCell& targetCell, bool valid);

  /// Draw faint grid lines over the entire workspace.
  void DrawGridOverlay();

  // ---- Data ----------------------------------------------------------------

  std::vector<WindowEntry> m_entries;

  // Reject-flash state
  bool   m_showRejectFlash = false;
  double m_rejectFlashStart = -100.0;
  GridCell m_rejectCell;
};

}  // namespace sysid
