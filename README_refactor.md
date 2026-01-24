# Firmware Refactor Plan

## Step-by-Step Migration Plan

### 1. Encapsulate Device State in a Struct
- **Why:** Groups all current settings (brightness, pattern, colors, etc) into a single struct, reducing global clutter and making future refactors easier.
- **How:**
  - Create a `DeviceState` struct (e.g., in `DeviceState.h`).
  - Move all global variables representing the current device state (not hardware constants) into this struct.
  - Update code to use `deviceState.brightness` instead of `GlobalBrightness`, etc.
  - Leave the BLE and pattern logic as-is for now.
- **Test:** Confirm the device still works as before (patterns, brightness, BLE, etc).

### 2. Use UserPreferences for Persistent Settings
- **Why:** Already implemented; using it will clean up the code that loads/saves settings.
- **How:**
  - In `main.cpp`, after setup, load preferences into `DeviceState` (or vice versa).
  - When settings change (e.g., via BLE), update both `DeviceState` and `UserPreferences`, and call `save()`.
- **Test:** Change a setting, reboot, and confirm it persists.

### 3. Encapsulate BLE Logic in BLEManager
- **Why:** BLE setup and characteristic handling is a big source of clutter. Encapsulating it will make `main.cpp` much cleaner and easier to follow.
- **How:**
  - Move BLE setup, characteristic creation, and event handling into a `BLEManager` class.
  - Expose methods for updating characteristics and handling writes.
  - Pass references to `DeviceState` and/or callbacks for when BLE events occur.
- **Test:** Confirm BLE still works (connect, change settings, etc).

### 4. Encapsulate Pattern Logic in PatternManager
- **Why:** Pattern switching and LED updating is another major area of complexity. Encapsulating it will make it easier to add new patterns and manage state.
- **How:**
  - Move pattern update logic into a `PatternManager` class.
  - Expose methods for setting the pattern, updating LEDs, etc.
  - Use `DeviceState` for current pattern info.
- **Test:** Confirm patterns still work and can be changed.

### 5. (Optional) Encapsulate Input Logic
- **Why:** If button/potentiometer handling is complex, encapsulate it for further clarity.
- **How:**
  - Move button and potentiometer code into an `InputManager` class.
  - Expose events/callbacks for button presses, etc.
- **Test:** Confirm input still works.

### 6. Refactor main.cpp to Use Managers
- **Why:** This is the payoff: `main.cpp` becomes a simple orchestrator, calling into your managers.
- **How:**
  - Replace direct logic in `main.cpp` with calls to your new classes.
  - The main loop should be simple: handle BLE, update patterns, check input, etc.
- **Test:** Full system test.

---

## Summary Table

| Step | Action                                 | Test/Goal                        |
|------|----------------------------------------|----------------------------------|
| 1    | Encapsulate device state in struct     | Device works as before           |
| 2    | Use UserPreferences for persistence    | Settings persist after reboot    |
| 3    | Encapsulate BLE logic in BLEManager    | BLE works as before              |
| 4    | Encapsulate pattern logic in PatternMgr| Patterns work as before          |
| 5    | (Opt) Encapsulate input logic          | Input works as before            |
| 6    | Refactor main.cpp to use managers      | Code is clean, system works      |

---

## Notes
- You can use singletons or static instances for managers if you want to avoid passing references everywhere.
- For memory efficiency, avoid virtual methods and dynamic allocation.
- Use references to share state between managers (e.g., `StateManager` updates `DeviceState` and notifies `BLEManager`).
