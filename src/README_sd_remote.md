# Remote SD Card Access via BLE Protocol

## Overview
This document outlines a protocol and architecture for remotely accessing and managing files on a microcontroller's SD card over Bluetooth Low Energy (BLE). The goal is to enable headless devices to provide file listing, reading, and management features to a remote client (such as a web app) in a robust, extensible, and type-agnostic way.

---

## 1. Why This Pattern?
- **Headless operation:** No screen or buttons required on the device.
- **REST-like API:** BLE characteristics and commands mimic REST endpoints and methods.
- **Chunked streaming:** Handles large files and directories by sending data in manageable chunks.
- **Extensible:** Can be expanded to support more commands and data types.

---

## 2. Protocol Design

### BLE Characteristics
- **Command Characteristic (write):**
  - Receives commands (e.g., `LIST`, `READ filename`, `DELETE filename`).
- **Data Characteristic (notify/read):**
  - Streams data chunks (file lists, file contents, etc.).
- **Status Characteristic (notify/read):**
  - Sends status updates (e.g., `OK`, `ERROR`, `DONE`).

### Command Examples
| Action         | BLE Write (Command Char) | BLE Notify/Read (Data Char) | Notes                        |
|----------------|-------------------------|-----------------------------|------------------------------|
| List files     | `LIST`                  | File list (CSV/JSON)        | Like `GET /files`            |
| Read file      | `READ filename`         | File data (chunks)          | Like `GET /files/filename`   |
| Delete file    | `DELETE filename`       | Status message              | Like `DELETE /files/filename`|
| Write file     | `WRITE filename` + data | Status message              | Like `POST /files/filename`  |

### Data Chunk Format
- **Type:** (e.g., FILE_LIST, FILE_DATA)
- **Sequence number:** For ordering
- **Payload:** Actual data (string, binary, etc.)
- **End flag:** Indicates last chunk

Example (JSON):
```json
{
  "type": "FILE_LIST",
  "seq": 1,
  "payload": "file1.txt,file2.txt,dir1/",
  "end": false
}
```

---

## 3. Device Architecture

- **FileStreamer:** Reads files/directories in chunks, manages state.
- **DataStreamer:** Streams any data type in chunks.
- **BLEManager:** Handles BLE events, routes commands, manages streamers.

### Pseudocode Example
```cpp
void onCommandReceived(String cmd) {
    if (cmd == "LIST") {
        fileStreamer.beginList("/");
    } else if (cmd.startsWith("READ ")) {
        String filename = cmd.substring(5);
        fileStreamer.beginRead(filename);
    }
    // ... other commands
}

void loop() {
    fileStreamer.update(); // Sends next chunk if needed
}
```

---

## 4. Extensibility & Use Cases
- Add authentication, file upload, directory creation, etc.
- Use for any data streaming (not just files): logs, sensor data, etc.
- Can be adapted for WiFi, USB, or other transports.

---

## 5. Summary Table
| Component      | Role                                 |
|----------------|--------------------------------------|
| Command Char   | Receives commands from client        |
| Data Char      | Sends data chunks to client          |
| Status Char    | Sends status/error messages          |
| FileStreamer   | Reads files/directories in chunks    |
| DataStreamer   | Streams any data in chunks           |
| BLEManager     | Orchestrates BLE and streamers       |

---

## 6. References & Inspiration
- Nordic UART Service (NUS)
- Adafruit Bluefruit File Transfer
- BLE DFU protocols
- REST API design patterns
