#pragma once

#include "SRTask.h"
#include "SRQueue.h"
#include "LogMessage.h"
#include "LogManager.h"
#include <SD.h>

/**
 * File operation types
 */
enum class FileOpType {
    WRITE_LOG,      // Write log message
    WRITE_FILE,     // Write data to file
    APPEND_FILE,    // Append data to file
    CREATE_FILE,    // Create new file
    DELETE_FILE,    // Delete file
    FLUSH_BUFFER    // Force flush current buffer
};

/**
 * File operation request structure
 */
struct FileOpRequest {
    FileOpType type;
    String filename;
    String data;
    bool isBinary;
    
    FileOpRequest() : type(FileOpType::WRITE_LOG), isBinary(false) {}
    
    FileOpRequest(FileOpType opType, const String& fname, const String& content, bool binary = false)
        : type(opType), filename(fname), data(content), isBinary(binary) {}
    
    // Helper constructors
    static FileOpRequest writeLog(const LogMessage& msg) {
        return FileOpRequest(FileOpType::WRITE_LOG, "", "", false);
    }
    
    static FileOpRequest writeFile(const String& fname, const String& content, bool binary = false) {
        return FileOpRequest(FileOpType::WRITE_FILE, fname, content, binary);
    }
    
    static FileOpRequest appendFile(const String& fname, const String& content, bool binary = false) {
        return FileOpRequest(FileOpType::APPEND_FILE, fname, content, binary);
    }
    
    static FileOpRequest createFile(const String& fname) {
        return FileOpRequest(FileOpType::CREATE_FILE, fname, "", false);
    }
    
    static FileOpRequest deleteFile(const String& fname) {
        return FileOpRequest(FileOpType::DELETE_FILE, fname, "", false);
    }
    
    static FileOpRequest flushBuffer() {
        return FileOpRequest(FileOpType::FLUSH_BUFFER, "", "", false);
    }
};

/**
 * SDWriterTask - FreeRTOS task for all SD card writing operations
 * 
 * Handles:
 * - Log message writing
 * - File streaming and writing
 * - Buffered operations for efficiency
 * - Queue-based file operations
 */
class SDWriterTask : public SRTask {
public:
    SDWriterTask(const char* logFilename = "/logs/srdriver.log", 
                 uint32_t stackSize = 8192,  // Increased for file operations
                 UBaseType_t priority = tskIDLE_PRIORITY + 2,
                 BaseType_t core = 0)  // Pin to core 0 (WiFi/BLE core)
        : SRTask("SDWriter", stackSize, priority, core),
          _logFilename(logFilename),
          _logQueue(32, "LogQueue"),  // 32 message queue
          _fileOpQueue(16, "FileOpQueue"),  // 16 file operation queue
          _writeBuffer(nullptr),
          _bufferSize(1024),  // Increased buffer size
          _bufferPos(0),
          _lastFlushTime(0),
          _flushIntervalMs(1000),  // Flush every second
          _currentFile(""),
          _currentFileHandle() {  // Use default constructor
        
        // Allocate write buffer
        _writeBuffer = new char[_bufferSize];
        if (_writeBuffer) {
            _writeBuffer[0] = '\0';
        }
    }
    
    ~SDWriterTask() {
        // Flush any remaining data and close files
        flushBuffer();
        closeCurrentFile();
        
        // Clean up buffer
        if (_writeBuffer) {
            delete[] _writeBuffer;
            _writeBuffer = nullptr;
        }
    }
    
    /**
     * Get the log queue for LogManager to use
     */
    SRQueue<LogMessage>* getLogQueue() {
        return &_logQueue;
    }
    
    /**
     * Get the file operation queue for other tasks to use
     */
    SRQueue<FileOpRequest>* getFileOpQueue() {
        return &_fileOpQueue;
    }
    
    /**
     * Set flush interval (how often to write to SD card)
     */
    void setFlushInterval(uint32_t intervalMs) {
        _flushIntervalMs = intervalMs;
    }
    
    /**
     * Force immediate flush of buffer to SD card
     */
    void forceFlush() {
        flushBuffer();
    }
    
    /**
     * Request a file operation (non-blocking)
     */
    bool requestFileOp(const FileOpRequest& request) {
        return _fileOpQueue.send(request);
    }
    
    /**
     * Request a file operation with timeout
     */
    bool requestFileOp(const FileOpRequest& request, uint32_t timeoutMs) {
        return _fileOpQueue.send(request, timeoutMs);
    }
    
    /**
     * Convenience methods for common file operations
     */
    bool writeFile(const String& filename, const String& data, bool binary = false) {
        return requestFileOp(FileOpRequest::writeFile(filename, data, binary));
    }
    
    bool appendFile(const String& filename, const String& data, bool binary = false) {
        return requestFileOp(FileOpRequest::appendFile(filename, data, binary));
    }
    
    bool createFile(const String& filename) {
        return requestFileOp(FileOpRequest::createFile(filename));
    }
    
    bool deleteFile(const String& filename) {
        return requestFileOp(FileOpRequest::deleteFile(filename));
    }

protected:
    /**
     * Main task loop - processes log messages and file operations
     */
    void run() override {
        // Set up the LogManager to use our queue
        LogManager::getInstance().setLogQueue(&_logQueue);
        
        LOG_INFO("SDWriterTask started");
        LOG_PRINTF("Writing logs to: %s", _logFilename.c_str());
        
        // Ensure log directory exists
        ensureLogDirectory();
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Process log messages
            processLogMessages();
            
            // Process file operations
            processFileOperations();
            
            // Check if it's time to flush
            uint32_t now = millis();
            if (now - _lastFlushTime >= _flushIntervalMs) {
                flushBuffer();
                _lastFlushTime = now;
            }
            
            // Sleep until next cycle (50ms for responsiveness)
            SRTask::sleepUntil(&lastWakeTime, 50);
        }
    }

private:
    /**
     * Process all available log messages from the queue
     */
    void processLogMessages() {
        LogMessage msg;
        while (_logQueue.receive(msg)) {
            // Format the log message
            String formattedMsg = formatLogMessage(msg);
            
            // Add to buffer (will be written to log file)
            addToBuffer(formattedMsg);
        }
    }
    
    /**
     * Process all available file operations from the queue
     */
    void processFileOperations() {
        FileOpRequest request;
        while (_fileOpQueue.receive(request)) {
            switch (request.type) {
                case FileOpType::WRITE_LOG:
                    // Already handled by processLogMessages()
                    break;
                    
                case FileOpType::WRITE_FILE:
                    writeFileData(request.filename, request.data, request.isBinary, false);
                    break;
                    
                case FileOpType::APPEND_FILE:
                    writeFileData(request.filename, request.data, request.isBinary, true);
                    break;
                    
                case FileOpType::CREATE_FILE:
                    createFileData(request.filename);
                    break;
                    
                case FileOpType::DELETE_FILE:
                    deleteFileData(request.filename);
                    break;
                    
                case FileOpType::FLUSH_BUFFER:
                    flushBuffer();
                    break;
            }
        }
    }
    
    /**
     * Write data to a file
     */
    void writeFileData(const String& filename, const String& data, bool binary, bool append) {
        // Close current file if it's different
        if (_currentFile != filename) {
            closeCurrentFile();
        }
        
        // Open file if needed
        if (!_currentFileHandle) {
            const char* mode = append ? FILE_WRITE : FILE_WRITE;
            _currentFileHandle = SD.open(filename.c_str(), mode);
            if (_currentFileHandle) {
                _currentFile = filename;
                LOG_DEBUGF("Opened file for writing: %s", filename.c_str());
            } else {
                LOG_ERRORF("Failed to open file for writing: %s", filename.c_str());
                return;
            }
        }
        
        // Write data
        size_t bytesWritten = _currentFileHandle.print(data);
        if (bytesWritten != data.length()) {
            LOG_WARNF("Incomplete write to file: %s", filename.c_str());
        }
        
        // Flush immediately for important files
        if (filename.startsWith("/logs/") || filename.startsWith("/data/")) {
            _currentFileHandle.flush();
        }
        
        LOG_DEBUGF("Wrote %d bytes to %s", bytesWritten, filename.c_str());
    }
    
    /**
     * Create a new file
     */
    void createFileData(const String& filename) {
        File file = SD.open(filename.c_str(), FILE_WRITE);
        if (file) {
            file.close();
            LOG_INFOF("Created file: %s", filename.c_str());
        } else {
            LOG_ERRORF("Failed to create file: %s", filename.c_str());
        }
    }
    
    /**
     * Delete a file
     */
    void deleteFileData(const String& filename) {
        if (SD.remove(filename.c_str())) {
            LOG_INFOF("Deleted file: %s", filename.c_str());
        } else {
            LOG_ERRORF("Failed to delete file: %s", filename.c_str());
        }
    }
    
    /**
     * Close the currently open file
     */
    void closeCurrentFile() {
        if (_currentFileHandle) {
            _currentFileHandle.close();
            _currentFileHandle = File();  // Use default constructor instead of nullptr
            _currentFile = "";
            LOG_DEBUG("Closed current file");
        }
    }
    
    /**
     * Format a log message for SD card writing
     */
    String formatLogMessage(const LogMessage& msg) {
        // Format: [timestamp] LEVEL: message
        return String("[") + String(msg.timestamp) + "] " + 
               msg.getLevelString() + ": " + msg.message + "\n";
    }
    
    /**
     * Add text to the write buffer (for log file)
     */
    void addToBuffer(const String& text) {
        if (!_writeBuffer) return;
        
        int textLen = text.length();
        int remainingSpace = _bufferSize - _bufferPos - 1;
        
        if (textLen <= remainingSpace) {
            // Fits in buffer
            strcpy(_writeBuffer + _bufferPos, text.c_str());
            _bufferPos += textLen;
        } else {
            // Buffer would overflow, flush first
            flushBuffer();
            
            // If text is larger than buffer, write directly
            if (textLen >= _bufferSize) {
                writeToLogFile(text);
            } else {
                // Add to fresh buffer
                strcpy(_writeBuffer, text.c_str());
                _bufferPos = textLen;
            }
        }
    }
    
    /**
     * Flush buffer contents to log file
     */
    void flushBuffer() {
        if (_bufferPos > 0 && _writeBuffer) {
            _writeBuffer[_bufferPos] = '\0';
            writeToLogFile(String(_writeBuffer));
            _bufferPos = 0;
            _writeBuffer[0] = '\0';
        }
    }
    
    /**
     * Write text directly to log file
     */
    void writeToLogFile(const String& text) {
        File logFile = SD.open(_logFilename.c_str(), FILE_WRITE);
        if (logFile) {
            size_t bytesWritten = logFile.print(text);
            logFile.close();
            
            if (bytesWritten != text.length()) {
                Serial.printf("[SDWriterTask] Warning: Incomplete write to log file\n");
            }
        } else {
            Serial.printf("[SDWriterTask] Error: Could not open log file %s\n", 
                        _logFilename.c_str());
        }
    }
    
    /**
     * Ensure the log directory exists
     */
    void ensureLogDirectory() {
        // Extract directory path from filename
        int lastSlash = _logFilename.lastIndexOf('/');
        if (lastSlash > 0) {
            String dirPath = _logFilename.substring(0, lastSlash);
            if (!SD.exists(dirPath.c_str())) {
                // Create directory (SD library should handle this)
                Serial.printf("[SDWriterTask] Note: Directory %s may need to exist\n", 
                            dirPath.c_str());
            }
        }
    }
    
    String _logFilename;
    SRQueue<LogMessage> _logQueue;
    SRQueue<FileOpRequest> _fileOpQueue;
    char* _writeBuffer;
    uint32_t _bufferSize;
    uint32_t _bufferPos;
    uint32_t _lastFlushTime;
    uint32_t _flushIntervalMs;
    String _currentFile;
    File _currentFileHandle;
}; 