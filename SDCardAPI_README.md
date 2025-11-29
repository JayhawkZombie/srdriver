# SDCard API Documentation

## Overview
The SDCard API provides a simple command interface for managing files on the SD card. Commands are sent via serial and follow a consistent format.

## Command Format
All commands follow the pattern: `COMMAND:param1:param2:...`

**Note**: Due to serial buffer limitations (~200-250 characters), commands and content are limited in size.

## Available Commands

### PRINT:filename
Prints the contents of a file to serial output.
- **Format**: `PRINT:filename`
- **Example**: `PRINT:test.txt`
- **Behavior**: Streams file content to serial when complete

### LIST
Lists all files on the SD card.
- **Format**: `LIST`
- **Example**: `LIST`
- **Behavior**: Outputs file listing to serial (currently placeholder)

### DELETE:filename
Deletes a file from the SD card.
- **Format**: `DELETE:filename`
- **Example**: `DELETE:oldfile.txt`
- **Behavior**: Removes file if it exists

### WRITE:filename:content
Creates or overwrites a file with the specified content.
- **Format**: `WRITE:filename:content`
- **Example**: `WRITE:test.txt:Hello World`
- **Behavior**: Overwrites entire file with new content
- **Limits**: Content limited by serial buffer (~200 chars)

### APPEND:filename:content
Appends content to an existing file.
- **Format**: `APPEND:filename:content`
- **Example**: `APPEND:log.txt:New entry\n`
- **Behavior**: Adds content to end of file
- **Limits**: Content limited by serial buffer (~200 chars)

### INFO:filename
Gets information about a file.
- **Format**: `INFO:filename`
- **Example**: `INFO:test.txt`
- **Output**: File size and basic info

## Data Limitations

### Serial Buffer Limits
- **ESP32 Serial Buffer**: 256 bytes default
- **Practical Command Limit**: ~200-250 characters total
- **Content Limit**: ~150-200 characters for WRITE/APPEND content

### File System Limits
- **File Size**: Limited by SD card capacity (typically 32GB+)
- **File Count**: Limited by SD card capacity
- **Filename Length**: 8.3 format (8 chars + 3 char extension)

## Usage Examples

### Basic File Operations
```
WRITE:hello.txt:Hello World!
INFO:hello.txt
PRINT:hello.txt
APPEND:hello.txt: - Goodbye!
DELETE:hello.txt
```

### Logging
```
WRITE:log.txt:Application started\n
APPEND:log.txt:Event 1 occurred\n
APPEND:log.txt:Event 2 occurred\n
PRINT:log.txt
```

### Error Handling
- Invalid commands return "ERROR: Unknown command"
- File not found returns "ERROR: File not found"
- Write failures return "ERROR: Failed to write"
- Busy operations return "ERROR: Another operation is in progress"

## Future Enhancements

### Planned Commands
- `MKDIR:dirname` - Create directory
- `RENAME:oldname:newname` - Rename file
- `COPY:source:dest` - Copy file
- `READ:filename:start:length` - Read specific portion
- `SEARCH:filename:pattern` - Search file contents

### Improvements Needed
1. **Large Content Support**: Implement chunked writes for large content
2. **Better LIST**: Return actual file listing instead of placeholder
3. **Directory Support**: Add directory navigation and management
4. **Binary Support**: Handle binary file operations
5. **BLE Integration**: Expose API over Bluetooth Low Energy

## Implementation Notes

### Architecture
- Uses `FileStreamer` for efficient file reading
- Uses `SDCardIndexer` for file listing
- Integrates with `TaskScheduler` for non-blocking operations
- Modular design with separate header/implementation files

### Error Handling
- All operations return success/error messages
- Busy state prevents concurrent operations
- File existence is checked before operations

### Performance
- Non-blocking operations using task scheduler
- Efficient streaming for large files
- Minimal memory footprint 