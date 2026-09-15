# Recoil Control

A lightweight, single-file Win32 desktop application for Windows that applies configurable mouse recoil compensation while you hold a mouse button or a key. Supports three movement stages (primary, secondary, tertiary), independent delay toggles, fine-grained sensitivity scaling, and a global master switch.

Built in pure Win32 / C++ with no external dependencies beyond the Windows SDK. Compiles to a single small `.exe`.

---

## Features

### Movement Stages
- **Primary Movement** — applied immediately on activation.
- **Secondary Movement** — replaces primary after *Delay 1* (optional).
- **Tertiary Movement** — replaces secondary after *Delay 2* (optional).
- Each stage has its own Left / Right / Up / Down values (0–100), so you can shape the recoil curve over time.

### Activation Modes
- **Hold Mouse 1** — active only while left mouse button is held.
- **Hold Mouse 1+2** — active only while both left and right mouse buttons are held.
- **Toggle Key** — press the activation key to toggle on, press again to toggle off.
- **Hold Key** — active only while the activation key is held down.

### Master Toggle
- A separate, globally hotkeyed **Master Toggle** enables or disables the entire tool regardless of activation mode.
- If the master toggle is off, no recoil is applied even when M1 / M1+M2 / the activation key is held.
- Toggle state is reflected in the status bar.

### Sensitivity Scaling
- A dedicated **Sensitivity** slider (0.01 – 10.00) scales the movement multiplier.
- Each slider step is small enough to fine-tune per-game recoil behavior without overshooting.
- Values are stored and displayed to two decimal places.

### Timing
- Delay 1 range: 100 – 5000 ms.
- Delay 2 range: 100 – 5000 ms (measured from activation, not from Delay 1 — tertiary triggers at `delay1 + delay2` when both stages are on).

### Configuration
- Save / Load settings to an `.ini` file via the native Windows file dialog.
- **New Config** resets everything to safe defaults.
- All settings (movement values, delays, toggle keys, activation mode, stage enables) are persisted.

### UI
- Modern Segoe UI font throughout.
- Grouped sections for Primary, Activation, Secondary, and Tertiary settings.
- Live value labels next to every slider.
- Real-time status display showing active state, current stage, and activation mode.

---

## Requirements

- Windows 10 or Windows 11 (should also work on Windows 8/8.1).
- Visual Studio 2019 / 2022 with the **Desktop development with C++** workload, or MinGW-w64.

No third-party libraries are required. Links against:
- `comctl32.lib`
- `winmm.lib`

---

## Building

### Visual Studio
1. Create a new **Empty C++ Project**.
2. Add `Source.cpp` (or rename it to `main.cpp`) to the project.
3. Set the project to **Release / x64** (or Win32).
4. Under **Project Properties → Linker → SubSystem**, set to **Windows (/SUBSYSTEM:WINDOWS)**.
5. Under **C/C++ → Preprocessor**, ensure `UNICODE` and `_UNICODE` are defined (optional but recommended).
6. Build.

### Command line (MSVC)
```bat
cl /EHsc /O2 /DUNICODE /D_UNICODE Source.cpp ^
   /link /SUBSYSTEM:WINDOWS comctl32.lib winmm.lib user32.lib gdi32.lib
