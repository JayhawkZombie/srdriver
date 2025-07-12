# File/Data Streaming over Serial (and Beyond)

## Overview
This document outlines a generic, protocol-agnostic file/data streaming system for microcontrollers. The initial implementation will use Serial (UART) for testing, with the goal of later adapting the same architecture for BLE or other transports. The system will support chunked streaming, state management, and easy extension to new protocols.

---

## 1. Why Start with Serial?
- **Easier debugging:** Serial is simple and well-supported.
- **Protocol-agnostic:** The same chunking and state logic can be reused for BLE, WiFi, etc.
- **Simulate client/server:** Serial monitor or script can "pretend" to be the browser or remote client.

---

## 2. Core Concepts
- **Chunked transfer:** Data (files, logs, etc.) is sent in small, fixed-size chunks.
- **Stateful streaming:** Streamer objects manage file position, chunking, and completion.
- **Command/response:** Client sends commands (LIST, READ, etc.), device responds with data/status.
- **Protocol abstraction:** The transport layer (Serial, BLE, etc.) is abstracted away from the streaming logic.
- **Co-routines for multitasking:** Use co-routines to interleave file streaming, LED updates, and other tasks without blocking.

---

## 3. FileStreamer/DataStreamer Class Design

### Responsibilities
- Open file or data source
- Read and send data in chunks
- Track position, sequence, and completion
- Handle commands (start, stop, pause, etc.)
- Report status/errors
- Yield control to allow other tasks (via co-routines)

### Example API
```cpp
class DataStreamer {
public:
    bool beginRead(const char* filename);
    void update(); // Call in loop(), sends next chunk if needed
    bool isDone() const;
    void stop();
    // ...
};
```

---

## 4. Serial Protocol Example

### Command/Response Flow
1. **Client:** Sends `READ filename` over Serial
2. **Device:** Opens file, starts streaming in chunks
3. **Device:** For each chunk, sends:
   - `DATA seq payload\n` (e.g., `DATA 1 abcdef...`)
4. **Device:** After last chunk, sends `DONE\n`
5. **Client:** Assembles data, confirms receipt

### Chunk Format
- `DATA <seq> <payload>`
- `DONE` (end of stream)
- `ERROR <msg>` (on failure)

---

## 5. Abstraction for Other Protocols
- Replace Serial send/receive with BLE notify/write, WiFi, etc.
- Keep chunking, state, and command logic unchanged.
- Enables easy migration to BLE or other transports.

---

## 6. Using Co-routines for Multitasking

### Why Co-routines?
- **Cleaner code:** Write logic that looks sequential but yields at key points.
- **No manual state machines:** Co-routines manage their own state.
- **Easy to interleave tasks:** Run file streaming, LED updates, and more as separate co-routines.

### Library Options
- **ArduinoCoroutine** ([awgrover/ArduinoCoroutine](https://github.com/awgrover/ArduinoCoroutine))
- **Protothreads** ([dunkels.com/adam/pt/](http://dunkels.com/adam/pt/))
- **TaskScheduler** ([arkhipenko/TaskScheduler](https://github.com/arkhipenko/TaskScheduler))
- **C++20 co-routines:** If your toolchain supports it (rare on microcontrollers, but possible on some modern platforms like ESP-IDF or with GCC 10+).

### C++20 Co-routines
- **Pros:** Native language support, very powerful.
- **Cons:** Not widely supported on Arduino/PlatformIO toolchains yet. Check your board and compiler version.
- **Recommendation:** Use a lightweight library for now; migrate to C++20 co-routines as support improves.

### Example Pseudocode with Co-routines
```cpp
Coroutine ledTask, fileTask;

void setup() {
    ledTask.begin(ledUpdateCoroutine);
    fileTask.begin(fileStreamingCoroutine);
}

void loop() {
    ledTask.run();
    fileTask.run();
}

void ledUpdateCoroutine(Coroutine& self) {
    while (true) {
        updateLEDs();
        FastLED.show();
        self.sleep(33); // 30 FPS
    }
}

void fileStreamingCoroutine(Coroutine& self) {
    while (true) {
        if (fileStreamingRequested) {
            while (file.hasData()) {
                sendNextChunk();
                self.sleep(5); // Yield for 5ms or next loop
            }
        }
        self.sleep(1); // Idle
    }
}
```

### FastLED and Co-routines
- **Caution:** FastLED.show() is timing-sensitive. Always yield before and after, never during.
- **Best practice:** Keep LED updates in their own co-routine, and ensure other tasks yield frequently.

---

## 7. Extensibility
- Add support for writing/uploading files
- Add pause/resume, progress reporting
- Add support for other data types (logs, sensor data, etc.)
- Add more co-routines for BLE, sensors, etc.

---

## 8. Testing & Debugging
- Use Serial Monitor or a Python script as the "client"
- Validate chunking, error handling, and state transitions
- Once stable, port to BLE or other protocols

---

## 9. Summary Table
| Component      | Role                                 |
|----------------|--------------------------------------|
| DataStreamer   | Reads/sends data in chunks           |
| Serial/BLE/... | Transport layer                      |
| Command Parser | Handles incoming commands            |
| State Machine  | Tracks streaming progress            |
| Co-routines    | Interleave tasks without blocking    |

---

## 10. Next Steps
- Choose a co-routine library (ArduinoCoroutine, Protothreads, etc.)
- Implement DataStreamer and LED update as co-routines
- Test with file reads and chunked transfer
- Abstract transport layer for BLE integration
- Monitor C++20 co-routine support for your toolchain
