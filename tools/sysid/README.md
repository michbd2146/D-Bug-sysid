# SysId: System Identification for Robot Mechanisms

> **D-Bug-sysid** — Enhanced fork by [@michbd2146](https://github.com/michbd2146)
> with additional analysis features built on top of WPILib's official SysId tool.

## New Features (D-Bug Enhancements)

### 🎛️ Bode Analysis & System Dynamics Card
After loading a data file and calculating gains, a **System Dynamics & Stability** section
is displayed showing:
- **Bandwidth (ωc)** in Hz and rad/s
- **Time Constant (τ)** in milliseconds
- **Continuous Pole location** in rad/s
- **95% Settling Time** in milliseconds

This gives you a MATLAB-style frequency-domain view of your mechanism's behavior.

### 📋 Copy as Code Button
A **"Copy as Code"** button appears in the Feedforward Analysis panel that generates
ready-to-paste Java and C++ code snippets containing your calculated gains:
- `SimpleMotorFeedforward` constructor (Java & C++)
- `PIDController` constructor (Java)
- CTRE Phoenix 6 `Slot0Configs` (Java / C++)
- REV SPARK Max `closedLoop.pid()` (Java)

### 🎨 Custom Theme
A polished UI theme for a more readable, visually distinct interface.

### 📊 Enhanced DataSelector
Improved data selection workflow with additional filtering and display options.

---

## Building and Running

See [requirements](../README.md#Requirements) for build prerequisites
(JDK 17+, Gradle, C++ toolchain).

Clone this fork and run from the repo root:

```bash
git clone https://github.com/michbd2146/D-Bug-sysid.git
cd D-Bug-sysid
git checkout sysid-enhancements
```

**Windows** — open a Developer Command Prompt for VS, or run:
```powershell
cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" && .\gradlew :tools:sysid:installSysidWindowsx86-64DebugExecutable"
```

Then launch the built executable:
```powershell
.\tools\sysid\build\install\sysid\windowsx86-64\debug\lib\sysid.exe
```

**Linux/macOS** — use `./gradlew` with the appropriate platform target.

---

## Troubleshooting

Use [AdvantageScope](https://docs.wpilib.org/en/stable/docs/software/dashboards/advantagescope.html)
(shipped with the WPILib installer) to view `.wpilog` files when SysId fails to generate plots.

---

## Credits

Based on [wpilibsuite/allwpilib](https://github.com/wpilibsuite/allwpilib).
All original WPILib code is licensed under the WPILib BSD license.
