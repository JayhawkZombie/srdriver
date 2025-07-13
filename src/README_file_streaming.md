# File/Data Streaming over Serial and BLE (Modernized)

## Overview
This document describes the current protocol for chunked, robust file/data streaming over Serial and BLE, as implemented in the project. The protocol is now JSON-based, with compact field names for efficiency and BLE compatibility.

---

## 1. Chunked Streaming Protocol (BLE/Serial)

### Envelope Format (per chunk)
Each chunk is a JSON object with the following fields:

| Field | Meaning                | Example Value |
|-------|------------------------|--------------|
| t     | Type of data           | "F" (file list), "D" (file data), etc. |
| s     | Sequence number        | 1            |
| n     | Total number of chunks | 6            |
| p     | Payload (string/base64)| "..."        |
| e     | End of stream?         | true/false   |

**Example:**
```json
{"t":"F","s":1,"n":6,"p":"{\"name\":\"/\",...","e":false}
```

- `t`: "F" for file list, "D" for file data, etc.
- `s`: Sequence number (starts at 1)
- `n`: Total number of chunks
- `p`: Payload (string for JSON, base64 for binary)
- `e`: true if this is the last chunk

### Why Compact Names?
- BLE notifications have strict size limits (often 80–180 bytes).
- Compact field names maximize payload per chunk.

---

## 2. Client Responsibilities
- Subscribe to the stream characteristic.
- Buffer and reassemble chunks by `s` (sequence number).
- When all `n` chunks are received (or `e: true`), concatenate `p` fields in order to reconstruct the full data.
- Parse the reassembled string as JSON (for file lists) or decode base64 (for binary data).

---

## 3. Extensibility
- Add new types by using different values for `t`.
- Add metadata fields as needed, but keep envelope compact.

---

## 4. Example Use Cases
- File/directory listing (`t: "F"`)
- File content streaming (`t: "D"`)
- Log streaming, sensor data, etc.

---

## 5. Implementation Notes
- Always ensure each chunk (including envelope) fits within BLE notification size (test with 80–160 bytes for max compatibility).
- All payloads must be valid UTF-8 (base64 for binary).
- No `delay()` in streaming loop; use task/coroutine scheduling.

---

## 6. Old Protocols (Removed)
- The previous chunking formats and non-JSON protocols are now deprecated and removed from this documentation.

---

## 7. Next Steps
- Keep envelope minimal for BLE.
- Add more types as needed.
- Keep this doc up to date with protocol changes.
