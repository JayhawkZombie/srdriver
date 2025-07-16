# Display System Architecture

## Overview

The SRDriver display system uses a double-buffered rendering approach similar to modern graphics pipelines. This separates rendering logic from display hardware, allowing any task to request display time while maintaining smooth updates.

## Current Architecture

### Components
- **SSD1306_Display**: Hardware driver for the 128x64 OLED display
- **DisplayQueue**: Manages banner messages and display requests
- **DisplayTask**: FreeRTOS task that updates the display at 30 FPS

### Current Limitations
- DisplayTask is overloaded with performance monitoring
- SystemMonitorTask has data but can't display it directly
- No separation between rendering and display management
- Limited flexibility for other tasks to display content

## Proposed Architecture: Double-Buffered Display System

### Core Design Philosophy
Following GPU practices, we separate:
- **Rendering API**: Drawing primitives that work on buffers
- **Frame Buffer**: In-memory representation of the display
- **Hardware Driver**: Uploads buffers to the physical display
- **Display Manager**: Coordinates which content gets displayed

### Key Components

#### 1. DisplayBuffer Class
```cpp
class DisplayBuffer {
    uint8_t buffer[128][64/8];  // Full 128x64 monochrome buffer
    bool dirty;                 // Content has changed
    
    // Drawing primitives (device-independent)
    void clear();
    void drawPixel(int x, int y, bool color);
    void drawLine(int x0, int y0, int x1, int y1, bool color);
    void drawText(int x, int y, const char* text, int size = 1);
    void drawRect(int x, int y, int w, int h, bool color);
    void drawCircle(int x, int y, int radius, bool color);
    void fillRect(int x, int y, int w, int h, bool color);
    
    // Convenience methods for main area (blue region)
    void drawMainAreaText(int x, int y, const char* text, int size = 1);
    void drawMainAreaRect(int x, int y, int w, int h, bool color);
    
    // Buffer management
    void markReady();
    bool isReady() const;
    const uint8_t* getBuffer() const;
};
```

#### 2. Extended DisplayQueue
```cpp
class DisplayQueue {
    // Banner messages (existing functionality)
    void requestBannerMessage(const String& taskName, const String& message, uint32_t timeout = 0);
    void clearBannerMessage(const String& taskName);
    
    // Main display area control (new)
    void requestMainDisplay(DisplayBuffer& buffer, const String& taskName);
    void releaseMainDisplay(const String& taskName);
    
    // Status queries
    bool hasActiveBanner() const;
    bool hasActiveMainDisplay() const;
    const String& getMainDisplayOwner() const;
    const DisplayBuffer* getCurrentMainDisplay() const;
    String getCurrentBannerText() const;
};
```

#### 3. Simplified DisplayTask
```cpp
class DisplayTask {
    void updateDisplay() {
        // 1. Render banner (always on top)
        renderBanner();
        
        // 2. Render main display area
        if (_queue.hasActiveMainDisplay()) {
            const DisplayBuffer* buffer = _queue.getCurrentMainDisplay();
            if (buffer && buffer->isReady()) {
                _display.uploadBuffer(buffer->getBuffer());
            }
        }
        
        // 3. Upload to hardware
        _display.show();
    }
};
```

#### 4. SystemMonitorTask Integration
```cpp
class SystemMonitorTask {
    DisplayBuffer _systemBuffer;
    
    void update() {
        // Update system statistics
        updateSystemStats();
        
        // Render to buffer
        _systemBuffer.clear();
        renderSystemStats(_systemBuffer);
        _systemBuffer.markReady();
        
        // Request display time
        DisplayQueue::getInstance().requestMainDisplay(_systemBuffer, "SystemMonitor");
    }
    
    void renderSystemStats(DisplayBuffer& buffer) {
        // CPU usage graph
        buffer.drawMainAreaText(2, 20, "CPU Usage:", 1);
        buffer.drawProgressBar(10, 30, 100, 8, getCpuUsage());
        
        // Memory usage
        buffer.drawMainAreaText(2, 45, "Memory:", 1);
        buffer.drawBarGraph(10, 55, 100, 8, getMemoryData(), 8);
        
        // Task count
        char taskText[32];
        snprintf(taskText, sizeof(taskText), "Tasks: %d", getTaskCount());
        buffer.drawMainAreaText(2, 65, taskText, 1);
    }
};
```

## Design Decisions

### Full Screen Buffer
- **Buffer size**: 128x64 pixels (1024 bytes)
- **Rationale**: Hardware independence, future flexibility
- **Convenience methods**: `drawMainArea*()` for blue region operations

### Banner Priority
- **Banner messages**: Always display over main content
- **Rationale**: Important notifications should be visible
- **Implementation**: Banner rendered last, overwrites main area

### No Fallback Content
- **Assumption**: SystemMonitorTask will always want to display
- **Future**: Priority system for multiple display requests
- **Current**: SystemMonitorTask is the primary main display owner

### Device Independence
- **Rendering API**: Works on buffers, not hardware
- **Hardware Driver**: Only handles buffer uploads
- **Benefits**: Easier testing, future hardware support

## Implementation Plan

### Phase 1: DisplayBuffer Class
- [ ] Create DisplayBuffer with drawing primitives
- [ ] Implement convenience methods for main area
- [ ] Add buffer management (dirty flag, ready state)

### Phase 2: Extend DisplayQueue
- [ ] Add main display request/release methods
- [ ] Implement display ownership tracking
- [ ] Add status query methods

### Phase 3: Simplify DisplayTask
- [ ] Remove performance monitoring from DisplayTask
- [ ] Implement buffer-based rendering
- [ ] Add buffer upload to hardware

### Phase 4: SystemMonitorTask Integration
- [ ] Add DisplayBuffer to SystemMonitorTask
- [ ] Implement system stats rendering
- [ ] Integrate with DisplayQueue

## Benefits

### Performance
- **Double buffering**: Prevents display tearing
- **Dirty flag**: Only upload when content changes
- **Simplified DisplayTask**: Faster update cycles

### Flexibility
- **Any task**: Can request display time
- **Rich rendering**: Full drawing API available
- **No timeouts**: Main display content persists

### Maintainability
- **Separation of concerns**: Clear component boundaries
- **Device independence**: Easy to test and modify
- **Extensible**: Easy to add new features

## Future Enhancements

### Priority System
- Multiple tasks requesting main display
- Priority-based display scheduling
- Time-sliced display sharing

### Multiple Regions
- Independent control of different screen areas
- Region-specific rendering APIs
- Layered display composition

### Hardware Abstraction
- Support for different display types
- SPI display support
- Multiple display support

## Technical Notes

### Memory Usage
- **DisplayBuffer**: 1024 bytes per buffer
- **Multiple buffers**: Tasks can have their own buffers
- **ESP32-S3**: Plenty of RAM for multiple buffers

### Performance Considerations
- **Buffer operations**: Fast in-memory operations
- **Upload time**: Still limited by I2C transfer (~25ms)
- **Rendering time**: Very fast (~1-2ms)

### Thread Safety
- **DisplayBuffer**: Not thread-safe (single owner)
- **DisplayQueue**: Thread-safe for coordination
- **DisplayTask**: Only task that uploads to hardware 