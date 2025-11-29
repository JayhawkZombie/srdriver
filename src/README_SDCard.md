# SD Card API Command Reference

This device supports the following SD card commands via serial or BLE:

## Command List

- **LIST [dir]**
  - List files and directories. If `dir` is omitted, lists root (`/`).
  - Example: `LIST` or `LIST /myfolder`

- **PRINT filename**
  - Print the contents of a file to serial.
  - Example: `PRINT data.txt`

- **DELETE filename**
  - Delete a file.
  - Example: `DELETE oldfile.txt`

- **WRITE filename:content**
  - Overwrite or create a file with the given content.
  - Example: `WRITE notes.txt:Hello World!`

- **APPEND filename:content**
  - Append content to an existing file.
  - Example: `APPEND log.txt:New entry`

- **INFO filename**
  - Get file information (size, etc.).
  - Example: `INFO data.txt`

- **MKDIR dirname**
  - Create a new directory.
  - Example: `MKDIR newfolder`

- **RMDIR dirname**
  - Remove a directory (must be empty).
  - Example: `RMDIR oldfolder`

- **TOUCH filename**
  - Create an empty file (or update its timestamp).
  - Example: `TOUCH empty.txt`

- **RENAME oldname:newname**
  - Rename a file or directory.
  - Example: `RENAME old.txt:new.txt`

- **EXISTS filename**
  - Check if a file or directory exists.
  - Example: `EXISTS data.txt`

- **MOVE oldname:newname**
  - Move a file or directory to a new location (rename or move between directories).
  - Example: `MOVE data.txt:/backup/data.txt`

- **COPY source:destination**
  - Copy a file to a new file.
  - Example: `COPY data.txt:data_backup.txt`

## Notes

- Use either a space or a colon to separate the command and its arguments.
- Filenames and directory names are case-sensitive.
- For commands with content (WRITE/APPEND), use a colon to separate the filename and content.
- For best results, set your serial monitor to send a newline (`\\n`) on Enter.
