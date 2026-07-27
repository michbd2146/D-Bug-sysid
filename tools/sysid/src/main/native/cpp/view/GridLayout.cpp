// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/sysid/view/GridLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <imgui_internal.h>  // ImGuiWindow, GImGui

#include "wpi/glass/Storage.hpp"

using namespace sysid;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ImVec2 GridLayout::CellSize() {
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  return ImVec2{disp.x / static_cast<float>(kGridCols),
                (disp.y - kMenuBarH) / static_cast<float>(kGridRows)};
}

ImVec2 GridLayout::CellToPixel(const GridCell& c) {
  const ImVec2 sz = CellSize();
  return ImVec2{c.col * sz.x, kMenuBarH + c.row * sz.y};
}

GridCell GridLayout::PixelToCell(ImVec2 px) {
  const ImVec2 sz = CellSize();
  GridCell c;
  c.col = static_cast<int>(std::floor(px.x / sz.x));
  c.row = static_cast<int>(std::floor((px.y - kMenuBarH) / sz.y));
  c.col = std::clamp(c.col, 0, kGridCols - 1);
  c.row = std::clamp(c.row, 0, kGridRows - 1);
  c.colSpan = 1;
  c.rowSpan = 1;
  return c;
}

GridCell GridLayout::GetSnappedCell(ImVec2 windowPos, int colSpan,
                                    int rowSpan) {
  const ImVec2 sz = CellSize();
  GridCell c;
  c.col = static_cast<int>(std::round(windowPos.x / sz.x));
  c.row = static_cast<int>(
      std::round((windowPos.y - kMenuBarH) / sz.y));
  c.colSpan = colSpan;
  c.rowSpan = rowSpan;

  // Clamp top-left origin so the window span stays within grid bounds
  c.col = std::clamp(c.col, 0, std::max(0, kGridCols - colSpan));
  c.row = std::clamp(c.row, 0, std::max(0, kGridRows - rowSpan));
  return c;
}

bool GridLayout::IsCellOccupied(const GridCell& c,
                                const WindowEntry* skip) const {
  for (const auto& e : m_entries) {
    if (&e == skip) {
      continue;
    }
    // AABB overlap check between cell 'c' (colSpan × rowSpan) and e.cell
    bool overlapCol = (c.col < e.cell.col + e.cell.colSpan) &&
                      (c.col + c.colSpan > e.cell.col);
    bool overlapRow = (c.row < e.cell.row + e.cell.rowSpan) &&
                      (c.row + c.rowSpan > e.cell.row);
    if (overlapCol && overlapRow) {
      return true;
    }
  }
  return false;
}

GridLayout::WindowEntry* GridLayout::FindOccupant(const GridCell& c,
                                                  const WindowEntry* skip) {
  for (auto& e : m_entries) {
    if (&e == skip) {
      continue;
    }
    bool overlapCol = (c.col < e.cell.col + e.cell.colSpan) &&
                      (c.col + c.colSpan > e.cell.col);
    bool overlapRow = (c.row < e.cell.row + e.cell.rowSpan) &&
                      (c.row + c.rowSpan > e.cell.row);
    if (overlapCol && overlapRow) {
      return &e;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Register / Reset
// ---------------------------------------------------------------------------

void GridLayout::Register(const char* name, GridCell defaultCell) {
  WindowEntry e;
  e.name        = name;
  e.defaultCell = defaultCell;
  e.cell        = defaultCell;
  e.forceReset  = true;
  m_entries.push_back(std::move(e));
}

void GridLayout::Reset() {
  for (auto& e : m_entries) {
    e.cell = e.defaultCell;
    e.wasDragging = false;
    e.wasResizing = false;
    e.lastSize    = {0, 0};
    e.forceReset  = true;
  }
  m_showRejectFlash = false;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void GridLayout::Save(wpi::glass::Storage& storage) {
  auto& child = storage.GetChild("GridLayout");
  for (const auto& e : m_entries) {
    auto& ws = child.GetChild(e.name);
    ws.GetInt("col",     e.cell.col);
    ws.GetInt("row",     e.cell.row);
    ws.GetInt("colSpan", e.cell.colSpan);
    ws.GetInt("rowSpan", e.cell.rowSpan);
    // Force overwrite stored values with current cell
    ws.SetInt("col",     e.cell.col);
    ws.SetInt("row",     e.cell.row);
    ws.SetInt("colSpan", e.cell.colSpan);
    ws.SetInt("rowSpan", e.cell.rowSpan);
  }
}

void GridLayout::Load(wpi::glass::Storage& storage) {
  auto& child = storage.GetChild("GridLayout");
  for (auto& e : m_entries) {
    auto& ws = child.GetChild(e.name);
    int col     = ws.GetInt("col",     e.defaultCell.col);
    int row     = ws.GetInt("row",     e.defaultCell.row);
    int colSpan = ws.GetInt("colSpan", e.defaultCell.colSpan);
    int rowSpan = ws.GetInt("rowSpan", e.defaultCell.rowSpan);
    col     = std::clamp(col,     0, kGridCols - 1);
    row     = std::clamp(row,     0, kGridRows - 1);
    colSpan = std::clamp(colSpan, 1, kGridCols - col);
    rowSpan = std::clamp(rowSpan, 1, kGridRows - row);
    e.cell = GridCell{col, row, colSpan, rowSpan};
  }
}

// ---------------------------------------------------------------------------
// Overlay drawing
// ---------------------------------------------------------------------------

void GridLayout::DrawDragPreview(const GridCell& targetCell, bool valid) {
  ImDrawList* dl     = ImGui::GetForegroundDrawList();
  const ImVec2 tl    = CellToPixel(targetCell);
  const ImVec2 sz    = CellSize();
  const ImVec2 br{
      tl.x + targetCell.colSpan * sz.x,
      tl.y + targetCell.rowSpan * sz.y};
  // Semi-transparent fill: green = valid, red = rejected
  const ImU32 fillCol = valid ? IM_COL32(80, 220, 80, 60)
                               : IM_COL32(220, 60, 60, 60);
  const ImU32 lineCol = valid ? IM_COL32(80, 220, 80, 200)
                               : IM_COL32(220, 60, 60, 200);
  dl->AddRectFilled(tl, br, fillCol);
  dl->AddRect(tl, br, lineCol, 0.0f, 0, 2.0f);
}

void GridLayout::DrawGridOverlay() {
  ImDrawList* dl      = ImGui::GetForegroundDrawList();
  const ImVec2 disp   = ImGui::GetIO().DisplaySize;
  const ImVec2 sz     = CellSize();
  const ImU32  col    = IM_COL32(255, 255, 255, 12);

  // Vertical lines
  for (int c = 0; c <= kGridCols; ++c) {
    float x = c * sz.x;
    dl->AddLine(ImVec2(x, kMenuBarH), ImVec2(x, disp.y), col);
  }
  // Horizontal lines
  for (int r = 0; r <= kGridRows; ++r) {
    float y = kMenuBarH + r * sz.y;
    dl->AddLine(ImVec2(0, y), ImVec2(disp.x, y), col);
  }
}

// ---------------------------------------------------------------------------
// Apply – per-frame engine
// ---------------------------------------------------------------------------

void GridLayout::Apply() {
  if (showGridOverlay) {
    DrawGridOverlay();
  }

  const ImVec2 cellSz = CellSize();
  const double now    = ImGui::GetTime();

  // Expire the reject flash overlay
  if (m_showRejectFlash &&
      (now - m_rejectFlashStart) > kRejectFlashSec) {
    m_showRejectFlash = false;
  }

  for (auto& e : m_entries) {
    ImGuiWindow* win = ImGui::FindWindowByName(e.name.c_str());
    if (!win || win->Hidden) {
      continue;
    }

    // ---- Detect drag & resize states ----------------------------------------
    bool isMouseDown   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool isMouseDrag   = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    bool isDragging    = (GImGui->MovingWindow == win);
    bool isInteracting = (GImGui->ActiveIdWindow == win);
    bool isResizing    = isInteracting && isMouseDrag && !isDragging;

    // ---- Detect drag END ---------------------------------------------------
    if (e.wasDragging && !isDragging && snapWhileDragging) {
      // Window was just released.  Snap the top-left corner to nearest grid cell.
      GridCell target = GetSnappedCell(win->Pos, e.cell.colSpan, e.cell.rowSpan);

      bool accepted = false;

      if (!IsCellOccupied(target, &e)) {
        // ---- Free cell → accept move ----------------------------------------
        e.cell   = target;
        accepted = true;
      } else {
        // ---- Occupied → attempt swap ----------------------------------------
        WindowEntry* other = FindOccupant(target, &e);
        if (other) {
          GridCell swapTarget = e.dragOrigin;
          // Check that swapping 'other' back to 'e.dragOrigin' is safe
          // (i.e. no third window sits there)
          bool swapClear = !IsCellOccupied(swapTarget, other);
          if (swapClear) {
            other->cell = swapTarget;
            e.cell      = target;
            accepted    = true;
          }
        }
      }

      if (!accepted) {
        // ---- Reject: snap back to origin + trigger red flash ---------------
        e.cell            = e.dragOrigin;
        m_showRejectFlash = true;
        m_rejectFlashStart = now;
        m_rejectCell      = target;
      }

      // Force the window to its resolved pixel position immediately
      ImGui::SetWindowPos(win, CellToPixel(e.cell), ImGuiCond_Always);
      ImGui::SetWindowSize(
          win,
          ImVec2(e.cell.colSpan * cellSz.x, e.cell.rowSpan * cellSz.y),
          ImGuiCond_Always);
    }

    // ---- Detect drag START or resize START (record origin) -----------------
    if ((!e.wasDragging && isDragging) || (!e.wasResizing && isResizing)) {
      e.dragOrigin = e.cell;
    }

    // ---- Detect resize END (snap all 4 edges to grid cells) ----------------
    if (e.wasResizing && !isMouseDown) {
      float leftPx   = win->Pos.x;
      float topPx    = win->Pos.y;
      float rightPx  = win->Pos.x + win->Size.x;
      float bottomPx = win->Pos.y + win->Size.y;

      int newCol       = static_cast<int>(std::round(leftPx / cellSz.x));
      int newRow       = static_cast<int>(std::round((topPx - kMenuBarH) / cellSz.y));
      int newRightCol  = static_cast<int>(std::round(rightPx / cellSz.x));
      int newBottomRow = static_cast<int>(std::round((bottomPx - kMenuBarH) / cellSz.y));

      newCol       = std::clamp(newCol, 0, kGridCols - 1);
      newRow       = std::clamp(newRow, 0, kGridRows - 1);
      newRightCol  = std::clamp(newRightCol, newCol + 1, kGridCols);
      newBottomRow = std::clamp(newBottomRow, newRow + 1, kGridRows);

      GridCell targetCell;
      targetCell.col     = newCol;
      targetCell.row     = newRow;
      targetCell.colSpan = newRightCol - newCol;
      targetCell.rowSpan = newBottomRow - newRow;

      if (!IsCellOccupied(targetCell, &e)) {
        e.cell = targetCell;
      } else {
        // Collision -> reject resize and snap back to origin
        e.cell            = e.dragOrigin;
        m_showRejectFlash = true;
        m_rejectFlashStart = now;
        m_rejectCell      = targetCell;
      }
    }

    // ---- Force position & size reset (on launch or after Reset Layout) -----
    if (e.forceReset) {
      ImGui::SetWindowPos(win, CellToPixel(e.cell), ImGuiCond_Always);
      ImGui::SetWindowSize(
          win,
          ImVec2(e.cell.colSpan * cellSz.x, e.cell.rowSpan * cellSz.y),
          ImGuiCond_Always);
      e.lastSize = ImVec2(e.cell.colSpan * cellSz.x, e.cell.rowSpan * cellSz.y);
      e.forceReset = false;
    }

    // ---- Per-frame position & size enforcement (non-dragging/resizing) -----
    if (!isDragging && !isResizing) {
      ImGui::SetWindowPos(win, CellToPixel(e.cell), ImGuiCond_Always);
      ImGui::SetWindowSize(
          win,
          ImVec2(e.cell.colSpan * cellSz.x, e.cell.rowSpan * cellSz.y),
          ImGuiCond_Always);
    }

    e.lastSize    = win->Size;
    e.wasDragging = isDragging;
    e.wasResizing = isResizing;

    // ---- Draw drag preview overlay (while dragging) -----------------------
    if (isDragging && snapWhileDragging) {
      GridCell previewCell =
          GetSnappedCell(win->Pos, e.cell.colSpan, e.cell.rowSpan);

      bool wouldCollide = IsCellOccupied(previewCell, &e);
      bool valid = true;
      if (wouldCollide) {
        // Check if a clean swap is possible
        WindowEntry* other = FindOccupant(previewCell, &e);
        valid = other && !IsCellOccupied(e.dragOrigin, other);
      }
      DrawDragPreview(previewCell, valid);
    }
  }

  // ---- Draw the reject flash overlay (brief red rectangle) ----------------
  if (m_showRejectFlash) {
    double elapsed = now - m_rejectFlashStart;
    float  alpha   = static_cast<float>(1.0 - elapsed / kRejectFlashSec);
    alpha          = std::clamp(alpha, 0.0f, 1.0f);
    DrawDragPreview(m_rejectCell, false);
    // Extra pulsing border
    ImDrawList* dl  = ImGui::GetForegroundDrawList();
    const ImVec2 tl = CellToPixel(m_rejectCell);
    const ImVec2 sz = CellSize();
    const ImVec2 br{
        tl.x + m_rejectCell.colSpan * sz.x,
        tl.y + m_rejectCell.rowSpan * sz.y};
    dl->AddRect(tl, br,
                IM_COL32(255, 60, 60, static_cast<int>(alpha * 255)),
                0.0f, 0, 3.0f);
  }
}
