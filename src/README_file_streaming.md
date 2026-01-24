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

## 1a. Example: Multi-Chunk JSON Payload

When a response (such as a file list or file content) is too large to fit in a single chunk, it is split across multiple envelopes. Each envelope contains a portion of the payload in the `p` field. The client must concatenate all `p` fields (in order of `s`) to reconstruct the full JSON string, then parse it.

**Suppose the full JSON payload is:**
```json
{"ok":1,"c":"L","d":"/logs","t":"d","ch":[{"f":"test.log","t":"f","sz":123,"ts":1717970000},{"f":"data2.txt","t":"f","sz":456,"ts":1717970100}],"ts":1717970200}
```

**This is split into two chunks:**

- **Chunk 1:**
```json
{"t":"F","s":1,"n":2,"p":"{\"ok\":1,\"c\":\"L\",\"d\":\"/logs\",\"t\":\"d\",\"ch\":[{\"f\":\"test.log\",\"t\":\"f\",\"sz\":123,\"ts\":1717970000}","e":false}
```

- **Chunk 2:**
```json
{"t":"F","s":2,"n":2,"p":",{\"f\":\"data2.txt\",\"t\":\"f\",\"sz\":456,\"ts\":1717970100}],\"ts\":1717970200}","e":true}
```

**Client reassembly:**
1. Buffer `p` from chunk 1: `{\"ok\":1,\"c\":\"L\",\"d\":\"/logs\",\"t\":\"d\",\"ch\":[{\"f\":\"test.log\",\"t\":\"f\",\"sz\":123,\"ts\":1717970000}`
2. Buffer `p` from chunk 2: `,{\"f\":\"data2.txt\",\"t\":\"f\",\"sz\":456,\"ts\":1717970100}],\"ts\":1717970200}`
3. Concatenate: `{\"ok\":1,\"c\":\"L\",\"d\":\"/logs\",\"t\":\"d\",\"ch\":[{\"f\":\"test.log\",\"t\":\"f\",\"sz\":123,\"ts\":1717970000},{\"f\":\"data2.txt\",\"t\":\"f\",\"sz\":456,\"ts\":1717970100}],\"ts\":1717970200}`
4. Parse as JSON (after unescaping):
```json
{"ok":1,"c":"L","d":"/logs","t":"d","ch":[{"f":"test.log","t":"f","sz":123,"ts":1717970000},{"f":"data2.txt","t":"f","sz":456,"ts":1717970100}],"ts":1717970200}
```

**Note:**
- The client must wait for all `n` chunks (or until `e: true`) before parsing the payload.
- The `p` field is always a string fragment; only after reassembly should it be parsed as JSON.

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

---

## 8. Compact JSON Response Fields for SD Card Commands

All SD card command responses are now JSON objects with compact field names to maximize BLE payload efficiency. Below are the standard fields used in responses:

| Field | Meaning                | Example Value                |
|-------|------------------------|------------------------------|
| ok    | Success (1) or error (0)| 1                            |
| c     | Command name (echoed)  | "LIST", "PRINT", etc.        |
| f     | Filename/path          | "/logs/test.log"             |
| d     | Directory path         | "/logs"                      |
| t     | Type                   | "f" (file), "d" (dir)        |
| sz    | Size (bytes)           | 123                          |
| ts    | Timestamp (UNIX epoch) | 1717970000                   |
| msg   | Message                | "Deleted"                    |
| ct    | Content (file data)    | "file contents here"         |
| ch    | Children (array)       | [ ... ]                      |
| ex    | Exists (1/0)           | 1                            |
| err   | Error message          | "File not found"             |
| fr    | From (source path)     | "/old/path.txt"              |
| to    | To (dest path)         | "/new/path.txt"              |

**Note:**
- The 'c' field in all responses is the full command name (e.g., "LIST", "PRINT", "INFO", etc.), not a single-letter code.
- Commands can be sent using the full, human-readable names.
- The device will echo the full command name in the response JSON.

### Example Responses

#### LIST
```json
{"ok":1,"c":"LIST","d":"/logs","t":"d","ch":[{"f":"test.log","t":"f","sz":123,"ts":1717970000},{"f":"data2.txt","t":"f","sz":456,"ts":1717970100}],"ts":1717970200}
```

#### PRINT
```json
{"ok":1,"c":"PRINT","f":"/logs/test.log","ct":"file contents here","ts":1717970000}
```

#### INFO
```json
{"ok":1,"c":"INFO","f":"/logs/test.log","sz":123,"t":"f","ts":1717970000}
```

#### DELETE
```json
{"ok":1,"c":"DELETE","f":"/logs/test.log","msg":"Deleted","ts":1717970000}
```

#### EXISTS
```json
{"ok":1,"c":"EXISTS","f":"/logs/test.log","ex":1,"ts":1717970000}
```

#### ERROR
```json
{"ok":0,"c":"DELETE","f":"/logs/test.log","err":"File not found","ts":1717970000}
```

---

*All responses are streamed in chunked envelopes as described above. The payload (`p`) of each chunk is a stringified compact JSON object as shown.*
