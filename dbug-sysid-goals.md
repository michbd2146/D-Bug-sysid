# D-Bug SysId Project Goals & Roadmap (`dbug-sysid-goals.md`)

This document outlines the planned feature roadmap and engineering goals for **D-Bug SysId**.

---

## 1. Grid-Based Movable & Resizable Widget Layout

> **Concept**: Implement a grid-snapping layout engine (similar to *Elastic Dashboard* or *Gridstack.js*) so users can fully customize their workspace without overlapping widgets.

### Key Requirements
- [ ] **Grid System**: Divide the workspace viewport into discrete grid cells (squares).
- [ ] **Single-Tile Occupancy**: Ensure only one widget occupies any given grid cell at a time.
- [ ] **Drag & Drop**: Smoothly drag widgets across grid cells with visual drop preview indicators.
- [ ] **Interactive Resizing**: Drag handles on widget borders to resize across multiple grid cells.
- [ ] **Auto-Reflow & Collision Handling**: Push neighboring widgets out of the way or swap positions when dropping into occupied cells.
- [ ] **Layout Persistence**: Save/Load custom grid configurations to `sysid.ini` or custom JSON layout presets.

---

## 2. Integrated On-Robot Routine Execution Tab

> **Concept**: Eliminate the need to manually deploy and trigger separate characterization code on the robot. Add a dedicated **Robot Control & Execution** tab inside SysId to trigger tests live over NetworkTables (NT4).

### Key Requirements
- [ ] **New Top-Level Tab**: Add a "Robot Test Runner" tab alongside the Analyzer/LogLoader.
- [ ] **Live NT4 Connection Status**: Connect to the RoboRIO IP (`10.TE.AM.2` or `169.254.x.x`) to verify status and safety interlocks.
- [ ] **Routine Triggering**:
  - [ ] Quasistatic Forward / Backward controls with live voltage limits.
  - [ ] Dynamic (Step Voltage) Forward / Backward controls with configurable step voltages.
- [ ] **Live Telemetry & Safety Cutoffs**:
  - [ ] Real-time display of mechanism position, velocity, and applied voltage during sweep execution.
  - [ ] Emergency Stop (E-Stop) button and automatic safety thresholds (e.g. max position limits, over-current trip).
- [ ] **Direct Data Handoff**: Automatically stream recorded test data directly into the *Data Selector* without requiring manual file downloading.

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
- [ ] **Code Snippet Generator Expansion**:
  - [ ] Update **Copy as Code** generator to support:
    - [ ] `VoltageOut` vs `TorqueCurrentFOC` snippets for CTRE Phoenix 6 (Java & C++).
    - [ ] `MotionMagicVoltage` vs `MotionMagicTorqueCurrentFOC` configuration blocks.

---

## Progress Log & Notes

- **Initial Setup**: Fork created at [`michbd2146/D-Bug-sysid`](https://github.com/michbd2146/D-Bug-sysid.git).
- **Branch**: `sysid-enhancements`
