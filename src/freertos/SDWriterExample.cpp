/**
 * SDWriterTask Integration Example
 * 
 * Shows how to use the expanded SDWriterTask for both logging and file operations.
 * This can replace your existing file streaming functionality with a task-based approach.
 */

#include "freertos/SRTask.h"
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"

// Global SD writer task instance
static SDWriterTask* g_sdWriterTask = nullptr;

/**
 * Initialize the SD writer task system
 * Call this from your setup() function
 */
void initSDWriterSystem() {
    Serial.println("[SDWriter] Initializing SD writer system...");
    
    // Create and start SD writer task
    g_sdWriterTask = new SDWriterTask("/logs/srdriver.log");
    if (g_sdWriterTask->start()) {
        Serial.println("[SDWriter] SD writer task started successfully");
        
        // Wait a moment for task to initialize
        delay(100);
        
        // Log system startup
        LOG_INFO("SD writer system initialized");
        LOG_PRINTF("Log file: /logs/srdriver.log");
        LOG_PRINTF("Buffer size: 1024 bytes");
        LOG_PRINTF("Flush interval: 1000ms");
        
    } else {
        Serial.println("[SDWriter] Failed to start SD writer task");
    }
}

/**
 * Clean up the SD writer system
 */
void cleanupSDWriterSystem() {
    if (g_sdWriterTask) {
        LOG_INFO("Shutting down SD writer system...");
        g_sdWriterTask->forceFlush();
        g_sdWriterTask->stop();
        delete g_sdWriterTask;
        g_sdWriterTask = nullptr;
    }
}

/**
 * Example: Replace your existing file streaming with task-based approach
 */
void exampleFileStreaming() {
    if (!g_sdWriterTask) {
        LOG_ERROR("SD writer task not initialized");
        return;
    }
    
    // Example 1: Write a data file
    String dataContent = "Hello, this is test data!\nLine 2\nLine 3";
    if (g_sdWriterTask->writeFile("/data/test.txt", dataContent)) {
        LOG_INFO("File write request queued: /data/test.txt");
    } else {
        LOG_ERROR("Failed to queue file write request");
    }
    
    // Example 2: Append to a log file
    String logEntry = "[" + String(millis()) + "] User action: button pressed\n";
    if (g_sdWriterTask->appendFile("/logs/user_actions.log", logEntry)) {
        LOG_DEBUG("Log append request queued");
    }
    
    // Example 3: Create a new file
    if (g_sdWriterTask->createFile("/data/new_file.txt")) {
        LOG_INFO("File creation request queued");
    }
    
    // Example 4: Write binary data (base64 encoded)
    String binaryData = "SGVsbG8gV29ybGQ=";  // "Hello World" in base64
    if (g_sdWriterTask->writeFile("/data/binary.dat", binaryData, true)) {
        LOG_INFO("Binary file write request queued");
    }
}

/**
 * Example: How to replace your existing PRINT command file streaming
 */
class FileStreamingExample {
public:
    /**
     * Stream a file to SD card (replacement for your existing file streaming)
     */
    void streamFileToSD(const String& sourcePath, const String& destPath) {
        if (!g_sdWriterTask) {
            LOG_ERROR("SD writer task not available");
            return;
        }
        
        LOG_INFOF("Starting file stream: %s -> %s", sourcePath.c_str(), destPath.c_str());
        
        // Open source file
        File sourceFile = SD.open(sourcePath.c_str(), FILE_READ);
        if (!sourceFile) {
            LOG_ERRORF("Could not open source file: %s", sourcePath.c_str());
            return;
        }
        
        // Create destination file
        if (!g_sdWriterTask->createFile(destPath)) {
            LOG_ERRORF("Could not create destination file: %s", destPath.c_str());
            sourceFile.close();
            return;
        }
        
        // Stream file in chunks
        const size_t chunkSize = 512;
        uint8_t buffer[chunkSize];
        size_t totalBytes = 0;
        
        while (sourceFile.available()) {
            size_t bytesRead = sourceFile.read(buffer, chunkSize);
            if (bytesRead > 0) {
                // Convert to string for queue transmission
                String chunkData = String((char*)buffer, bytesRead);
                
                // Queue the chunk for writing
                if (g_sdWriterTask->appendFile(destPath, chunkData)) {
                    totalBytes += bytesRead;
                    LOG_DEBUGF("Queued chunk: %d bytes (total: %d)", bytesRead, totalBytes);
                } else {
                    LOG_ERROR("Failed to queue file chunk");
                    break;
                }
            }
        }
        
        sourceFile.close();
        LOG_INFOF("File streaming complete: %d bytes", totalBytes);
    }
    
    /**
     * Write a large file in chunks (for BLE file uploads)
     */
    void writeLargeFile(const String& filename, const String& data, size_t chunkSize = 512) {
        if (!g_sdWriterTask) {
            LOG_ERROR("SD writer task not available");
            return;
        }
        
        LOG_INFOF("Writing large file: %s (%d bytes)", filename.c_str(), data.length());
        
        // Create the file
        if (!g_sdWriterTask->createFile(filename)) {
            LOG_ERRORF("Could not create file: %s", filename.c_str());
            return;
        }
        
        // Write in chunks
        size_t offset = 0;
        size_t chunkCount = 0;
        
        while (offset < data.length()) {
            size_t chunkLength = min(chunkSize, data.length() - offset);
            String chunk = data.substring(offset, offset + chunkLength);
            
            if (g_sdWriterTask->appendFile(filename, chunk)) {
                offset += chunkLength;
                chunkCount++;
                
                if (chunkCount % 10 == 0) {  // Log progress every 10 chunks
                    LOG_DEBUGF("File write progress: %d/%d bytes (%d chunks)", 
                              offset, data.length(), chunkCount);
                }
            } else {
                LOG_ERROR("Failed to queue file chunk");
                break;
            }
        }
        
        LOG_INFOF("Large file write complete: %d chunks, %d bytes", chunkCount, offset);
    }
};

/**
 * Example: How to integrate with your existing SDCardAPI
 */
class SDCardAPIWithTask {
public:
    /**
     * Replace your existing printFile method to use task-based writing
     */
    void printFileWithTask(const String& filename) {
        LOG_INFOF("Processing PRINT command: %s", filename.c_str());
        
        // Read the file
        File file = SD.open(filename.c_str(), FILE_READ);
        if (!file) {
            LOG_ERRORF("Could not open file for PRINT: %s", filename.c_str());
            return;
        }
        
        // Read file content
        String fileContent = "";
        while (file.available()) {
            fileContent += (char)file.read();
        }
        file.close();
        
        LOG_INFOF("File read complete: %d bytes", fileContent.length());
        
        // Queue for writing to a copy file (or process as needed)
        String copyPath = "/copies/" + filename.substring(filename.lastIndexOf('/') + 1);
        if (g_sdWriterTask->writeFile(copyPath, fileContent)) {
            LOG_INFOF("File copy queued: %s", copyPath.c_str());
        } else {
            LOG_ERROR("Failed to queue file copy");
        }
    }
    
    /**
     * Handle BLE file upload with task-based writing
     */
    void handleBLEFileUpload(const String& filename, const String& base64Data) {
        LOG_INFOF("Handling BLE file upload: %s", filename.c_str());
        
        // Decode base64 data
        String decodedData = base64Decode(base64Data);
        
        // Queue for writing
        if (g_sdWriterTask->writeFile(filename, decodedData)) {
            LOG_INFOF("BLE file upload queued: %s (%d bytes)", 
                     filename.c_str(), decodedData.length());
        } else {
            LOG_ERROR("Failed to queue BLE file upload");
        }
    }
    
private:
    // Simple base64 decode (you might want to use a proper library)
    String base64Decode(const String& input) {
        // This is a placeholder - implement proper base64 decoding
        return input;  // For now, just return as-is
    }
};

/**
 * Example: How to monitor the SD writer task
 */
void monitorSDWriterTask() {
    if (!g_sdWriterTask) return;
    
    // Get queue status
    SRQueue<FileOpRequest>* fileOpQueue = g_sdWriterTask->getFileOpQueue();
    if (fileOpQueue) {
        uint32_t itemCount = fileOpQueue->getItemCount();
        uint32_t spacesAvailable = fileOpQueue->getSpacesAvailable();
        
        LOG_DEBUGF("File operation queue: %d items, %d spaces available", 
                  itemCount, spacesAvailable);
        
        // Warn if queue is getting full
        if (spacesAvailable < 4) {
            LOG_WARN("File operation queue is getting full");
        }
    }
    
    // Check if task is still running
    if (!g_sdWriterTask->isRunning()) {
        LOG_ERROR("SD writer task has stopped unexpectedly");
        // You might want to restart it here
    }
}

/**
 * Example: How to use in your main loop
 */
void exampleMainLoopWithSDWriter() {
    // Initialize the system
    // initSDWriterSystem();  // Call this in setup()
    
    // Your existing loop becomes much simpler
    while (true) {
        // Monitor the SD writer task
        monitorSDWriterTask();
        
        // Your other tasks can now use the SD writer without blocking
        // exampleFileStreaming();  // This won't block your main loop
        
        // Sleep to allow other tasks to run
        SRTask::sleep(1000);
    }
}

/**
 * Example: How to replace your existing logging calls
 */
void exampleLoggingMigration() {
    // OLD: Serial.println("Processing command: PRINT");
    // NEW: LOG_INFO("Processing command: PRINT");
    
    // OLD: Serial.printf("Heap: %d bytes\n", ESP.getFreeHeap());
    // NEW: LOG_PRINTF("Heap: %d bytes", ESP.getFreeHeap());
    
    // OLD: Serial.println("Error: Could not open file");
    // NEW: LOG_ERROR("Could not open file");
    
    // OLD: Serial.printf("File size: %d bytes\n", fileSize);
    // NEW: LOG_PRINTF("File size: %d bytes", fileSize);
    
    // The SD writer task will automatically:
    // - Buffer log messages
    // - Write them to the SD card efficiently
    // - Not block your main loop
} 