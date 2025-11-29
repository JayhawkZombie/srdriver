#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * SRQueue - Template wrapper for FreeRTOS queues
 * 
 * Provides a type-safe, C++ interface for FreeRTOS queues.
 * Supports sending/receiving data with optional timeouts.
 */
template<typename T>
class SRQueue {
public:
    /**
     * Create a new queue
     * @param length Maximum number of items in the queue
     * @param name Optional name for debugging
     */
    SRQueue(UBaseType_t length, const char* name = nullptr)
        : _handle(nullptr), _name(name ? name : "unnamed") {
        _handle = xQueueCreate(length, sizeof(T));
        if (_handle) {
            Serial.printf("[SRQueue] Created queue '%s' with length %d\n", _name, length);
        } else {
            Serial.printf("[SRQueue] Failed to create queue '%s'\n", _name);
        }
    }

    ~SRQueue() {
        if (_handle) {
            vQueueDelete(_handle);
            Serial.printf("[SRQueue] Deleted queue '%s'\n", _name);
        }
    }

    /**
     * Send an item to the queue (non-blocking)
     * @param item Item to send
     * @return true if item was sent successfully
     */
    bool send(const T& item) {
        return send(item, 0);
    }

    /**
     * Send an item to the queue with timeout
     * @param item Item to send
     * @param timeoutMs Timeout in milliseconds (0 = non-blocking, portMAX_DELAY = infinite)
     * @return true if item was sent successfully
     */
    bool send(const T& item, uint32_t timeoutMs) {
        if (!_handle) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        BaseType_t result = xQueueSend(_handle, &item, timeout);
        return (result == pdTRUE);
    }

    /**
     * Send an item to the front of the queue (non-blocking)
     * @param item Item to send
     * @return true if item was sent successfully
     */
    bool sendToFront(const T& item) {
        return sendToFront(item, 0);
    }

    /**
     * Send an item to the front of the queue with timeout
     * @param item Item to send
     * @param timeoutMs Timeout in milliseconds
     * @return true if item was sent successfully
     */
    bool sendToFront(const T& item, uint32_t timeoutMs) {
        if (!_handle) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        BaseType_t result = xQueueSendToFront(_handle, &item, timeout);
        return (result == pdTRUE);
    }

    /**
     * Receive an item from the queue (non-blocking)
     * @param item Reference to store received item
     * @return true if item was received successfully
     */
    bool receive(T& item) {
        return receive(item, 0);
    }

    /**
     * Receive an item from the queue with timeout
     * @param item Reference to store received item
     * @param timeoutMs Timeout in milliseconds (0 = non-blocking, portMAX_DELAY = infinite)
     * @return true if item was received successfully
     */
    bool receive(T& item, uint32_t timeoutMs) {
        if (!_handle) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        BaseType_t result = xQueueReceive(_handle, &item, timeout);
        return (result == pdTRUE);
    }

    /**
     * Peek at the next item without removing it
     * @param item Reference to store peeked item
     * @param timeoutMs Timeout in milliseconds
     * @return true if item was peeked successfully
     */
    bool peek(T& item, uint32_t timeoutMs = 0) {
        if (!_handle) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        BaseType_t result = xQueuePeek(_handle, &item, timeout);
        return (result == pdTRUE);
    }

    /**
     * Get number of items currently in the queue
     */
    UBaseType_t getItemCount() const {
        return _handle ? uxQueueMessagesWaiting(_handle) : 0;
    }

    /**
     * Get number of free spaces in the queue
     */
    UBaseType_t getSpacesAvailable() const {
        return _handle ? uxQueueSpacesAvailable(_handle) : 0;
    }

    /**
     * Check if queue is empty
     */
    bool isEmpty() const {
        return getItemCount() == 0;
    }

    /**
     * Check if queue is full
     */
    bool isFull() const {
        return getSpacesAvailable() == 0;
    }

    /**
     * Reset the queue (remove all items)
     */
    void reset() {
        if (_handle) {
            xQueueReset(_handle);
        }
    }

    /**
     * Get queue handle (for advanced usage)
     */
    QueueHandle_t getHandle() const { return _handle; }

    /**
     * Get queue name
     */
    const char* getName() const { return _name; }

private:
    QueueHandle_t _handle;
    const char* _name;
}; 