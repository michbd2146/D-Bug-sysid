# D-Bug SysId Project Goals & Roadmap (`dbug-sysid-goals.md`)

This document outlines the planned feature roadmap and engineering goals for **D-Bug SysId**.

**Current Version**: `v2.0.0`

---

## 1. Grid-Based Movable & Resizable Widget Layout [COMPLETED - v2.0.0]

> **Concept**: Implement a grid-snapping layout engine (similar to *Elastic Dashboard* or *Gridstack.js*) so users can fully customize their workspace without overlapping widgets.

### Key Requirements
- [x] **Grid System**: Divide the workspace viewport into discrete grid cells (squares).
- [x] **Single-Tile Occupancy**: Ensure only one widget occupies any given grid cell at a time.
- [x] **Drag & Drop**: Smoothly drag widgets across grid cells with visual drop preview indicators.
- [x] **Interactive Resizing**: Drag handles on widget borders to resize across multiple grid cells.
- [x] **Auto-Reflow & Collision Handling**: Push neighboring widgets out of the way or swap positions when dropping into occupied cells.
- [x] **Layout Persistence**: Save/Load custom grid configurations to `sysid.ini` or custom JSON layout presets.

---

## 2. Integrated On-Robot Routine Execution Tab [COMPLETED - v2.1.0]

> **Concept**: Eliminate the need to manually deploy and trigger separate characterization code on the robot. Add a dedicated **Robot Control & Execution** tab inside SysId to trigger tests live over NetworkTables (NT4).

### Key Requirements
- [x] **New Top-Level Tab**: Add a "Robot Test Runner" panel alongside the Analyzer/LogLoader.
- [x] **Live NT4 Connection Status**: Connect to the RoboRIO IP (`10.TE.AM.2` or `169.254.x.x`) to verify status and safety interlocks.
- [x] **Routine Triggering**:
  - [x] Quasistatic Forward / Backward controls with live voltage limits.
  - [x] Dynamic (Step Voltage) Forward / Backward controls with configurable step voltages.
- [x] **Live Telemetry & Safety Cutoffs**:
  - [x] Real-time display of mechanism position, velocity, and applied voltage during sweep execution.
  - [x] Emergency Stop (E-Stop) button and automatic safety thresholds (e.g. max position limits, over-current trip).
- [x] **Direct Data Handoff**: Automatically stream recorded test data directly into the *Data Selector* without requiring manual file downloading.

---

## 3. High-Level Control, Torque Current & CTRE Motion Magic / FOC

> **Concept**: Expand SysId beyond traditional voltage-based ($V$) feedforwards to support modern motor controller modes like Torque Current Control ($A$), CTRE FOC (Field Oriented Control), and Motion Magic parameters.

### Key Requirements
- [ ] **Torque Current (Amps) Control Mode**:
  - [ ] Support characterization in terms of Torque Current ($I = K_s + K_v \cdot v + K_a \cdot a$) instead of voltage.
  - [ ] Display output gains in Amperes ($A$), $A / (\text{units/s})$, $A / (\text{units/s}^2)$.
- [ ] **CTRE Phoenix 6 Motion Magic & FOC Integration**:
  - [ ] Calculate direct `Slot0Configs` / `MotionMagicConfigs` values for CTRE Phoenix 6 (Talon FX / Krazy / Kraken X60).
  - [ ] Export FOC-specific Feedforward ($kS, kV, kA$) and Feedback ($kP, kI, kD$) gains tuned for TorqueCurrentFOC and VoltageFOC control requests.
- [ ] **REV SPARK Flex / SPARK Max Smart Motion**:
  - [ ] Gain translation for REV SPARK Flex/Max onboard closed-loop control modes (Duty Cycle vs. Voltage vs. Current modes).
- [ ] **Transfer Function ($G(s)$ & $G(z)$) & Model Analysis**:
  - [ ] **Continuous-Time Transfer Function**: Automatically derive $G(s) = \frac{K}{\tau s + 1}$ (velocity) or $G(s) = \frac{K}{s(\tau s + 1)}$ (position) from identified $K_v, K_a$ parameters.
  - [ ] **Key System Metrics Display**: Render formatted mathematical transfer function expressions $G(s)$, Mechanical Time Constant ($\tau = K_a / K_v$), System Gain ($K = 1/K_v$), and Bandwidth ($\omega_c = 1/\tau$) in the Analyzer GUI.
  - [ ] **Discrete-Time $G(z)$ & Sampling Delay**: Convert $G(s) \to G(z)$ via Zero-Order Hold (ZOH) at configurable sample rates (e.g. 20ms RoboRIO loop vs 1ms motor controller loop).
  - [ ] **Bode Theoretical Overlay**: Plot theoretical $G(s)$ magnitude & phase response directly over empirical chirp data in `BodeAnalysis` to highlight structural resonances or belt flex.
- [ ] **Code Snippet Generator Expansion**:
  - [ ] Update **Copy as Code** generator to support:
    - [ ] `VoltageOut` vs `TorqueCurrentFOC` snippets for CTRE Phoenix 6 (Java & C++).
    - [ ] `MotionMagicVoltage` vs `MotionMagicTorqueCurrentFOC` configuration blocks.
    - [ ] WPILib `LinearSystemId` / `LinearSystem` plant matrix ($A, B, C, D$) snippet export for Java & C++ robot code and simulation.

---

## Progress Log & Notes

- **Initial Setup**: Fork created at [`michbd2146/D-Bug-sysid`](https://github.com/michbd2146/D-Bug-sysid.git).
- **Branch**: `sysid-enhancements`
- **v1.0.0 (2026-07-27)**: Completed Goal 1 initial implementation (`GridLayout` engine, dynamic cell scaling, drag/drop preview, single-occupancy swap/reject, interactive resize snapping, grid settings menu, and persistence).
- **v1.0.1 (2026-07-27)**: Fixed ghost preview alignment to copy the exact top-left starting position of the dragged widget; fixed multi-cell AABB collision detection.
- **v1.0.2 (2026-07-27)**: Ensured every widget starts at its default spot and default size upon launch (`forceReset`), and configured the Reset button (`Widgets -> Reset Layout`) to restore all widgets to default.
- **v1.0.3 (2026-07-27)**: Enforced per-frame `SetWindowSize` alongside `SetWindowPos` when idle, ensuring instant reset for dimensions of all 5 widgets.
- **v1.0.4 (2026-07-27)**: Fixed interactive border resizing using `GImGui->ActiveIdWindow` and mouse dragging state.
- **v1.0.5 (2026-07-27)**: Added omnidirectional resizing support across all 8 borders and corners (Top, Bottom, Left, Right, Top-Left, Top-Right, Bottom-Left, Bottom-Right) with grid cell edge snapping and collision rejection.
- **v2.0.0 (2026-07-27)**: **Milestone Release!** Goal 1 is fully completed, verified, and accepted by the user. Version bumped to `v2.0.0` marking the completion of Goal 1 and readiness for Goal 2.
- **v2.1.0 (2026-07-29)**: **Goal 2 Completed!** Implemented `RobotRunner` & `RobotConnection` panel for live NetworkTables (NT4) on-robot routine execution, automated position/current safety cutoffs, Emergency Stop, and grid layout registration. All 40,664 unit test assertions verified successfully.
- **v2.2.0 (2026-07-29)**: **Safety Tab Workspace & Layout Lock!** Converted Robot Test Runner into a full-workspace locked Safety Tab Mode (`gAppMode == 1`), moved Mode Switcher tabs to the right side of the control bar, and enforced `ImGuiCond_Always` window positioning so it opens cleanly at `(5, 30)` and resets with `Widgets -> Reset Layout`.
