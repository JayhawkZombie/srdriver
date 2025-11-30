#pragma once

#include <Arduino.h>
#include <queue>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "LogManager.h"
/**
 * SRSmartQueue - Thread-safe queue that supports smart pointers and move semantics
 * 
 * Uses FreeRTOS mutex for thread safety and std::queue for storage.
 * This allows queuing of non-POD types like std::shared_ptr and std::unique_ptr.
 * 
 * Note: This is slightly less efficient than SRQueue (mutex overhead vs memcpy),
 * but enables safe queuing of complex types.
 */
template<typename T>
class SRSmartQueue {
public:
    /**
     * Create a new smart queue
     * @param maxLength Maximum number of items in the queue (0 = unlimited)
     * @param name Optional name for debugging
     */
    SRSmartQueue(UBaseType_t maxLength = 0, const char* name = nullptr)
        : _maxLength(maxLength), _name(name ? name : "unnamed") {
        _mutex = xSemaphoreCreateMutex();
        if (_mutex) {
            LOG_INFOF_COMPONENT("SRSmartQueue", "Created queue '%s' with max length %d", _name, maxLength);
        } else {
            LOG_ERRORF_COMPONENT("SRSmartQueue", "Failed to create mutex for queue '%s'", _name);
        }
    }

    ~SRSmartQueue() {
        if (_mutex) {
            // Clear queue before destroying mutex
            clear();
            vSemaphoreDelete(_mutex);
            LOG_INFOF_COMPONENT("SRSmartQueue", "Deleted queue '%s'", _name);
        }
    }

    /**
     * Send an item to the queue (non-blocking)
     * Supports both copy and move semantics
     * @param item Item to send (can be rvalue reference for move)
     * @return true if item was sent successfully
     */
    template<typename U>
    bool send(U&& item) {
        return send(std::forward<U>(item), 0);
    }

    /**
     * Send an item to the queue with timeout
     * Supports both copy and move semantics
     * @param item Item to send (can be rvalue reference for move)
     * @param timeoutMs Timeout in milliseconds (0 = non-blocking, portMAX_DELAY = infinite)
     * @return true if item was sent successfully
     */
    template<typename U>
    bool send(U&& item, uint32_t timeoutMs) {
        if (!_mutex) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        // Try to acquire mutex
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            return false;  // Timeout
        }
        
        // Check if queue is full
        if (_maxLength > 0 && _queue.size() >= _maxLength) {
            xSemaphoreGive(_mutex);
            return false;  // Queue full
        }
        
        // Add item to queue (move if rvalue, copy if lvalue)
        _queue.push(std::forward<U>(item));
        
        xSemaphoreGive(_mutex);
        return true;
    }

    /**
     * Send an item to the front of the queue (non-blocking)
     * Supports both copy and move semantics
     * @param item Item to send (can be rvalue reference for move)
     * @return true if item was sent successfully
     */
    template<typename U>
    bool sendToFront(U&& item) {
        return sendToFront(std::forward<U>(item), 0);
    }

    /**
     * Send an item to the front of the queue with timeout
     * Supports both copy and move semantics
     * @param item Item to send (can be rvalue reference for move)
     * @param timeoutMs Timeout in milliseconds
     * @return true if item was sent successfully
     */
    template<typename U>
    bool sendToFront(U&& item, uint32_t timeoutMs) {
        if (!_mutex) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        // Try to acquire mutex
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            return false;  // Timeout
        }
        
        // Check if queue is full
        if (_maxLength > 0 && _queue.size() >= _maxLength) {
            xSemaphoreGive(_mutex);
            return false;  // Queue full
        }
        
        // For front insertion, we need to use a deque or manually reorder
        // Since std::queue doesn't support front insertion, we'll use a workaround:
        // Create a temporary queue, add new item, then copy all old items
        std::queue<T> tempQueue;
        tempQueue.push(std::forward<U>(item));
        while (!_queue.empty()) {
            tempQueue.push(std::move(_queue.front()));
            _queue.pop();
        }
        _queue = std::move(tempQueue);
        
        xSemaphoreGive(_mutex);
        return true;
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
        if (!_mutex) return false;
        const auto startTime = micros();
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        // Try to acquire mutex
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            return false;  // Timeout
        }
        
        // Check if queue is empty
        if (_queue.empty()) {
            xSemaphoreGive(_mutex);
            return false;  // Queue empty
        }
        
        // Get item from front of queue
        item = std::move(_queue.front());
        _queue.pop();
        
        xSemaphoreGive(_mutex);
        const auto endTime = micros();
        const auto duration = endTime - startTime;
        return true;
    }

    /**
     * Peek at the next item without removing it
     * @param item Reference to store peeked item
     * @param timeoutMs Timeout in milliseconds
     * @return true if item was peeked successfully
     */
    bool peek(T& item, uint32_t timeoutMs = 0) {
        if (!_mutex) return false;
        
        TickType_t timeout = (timeoutMs == portMAX_DELAY) ? 
                            portMAX_DELAY : (timeoutMs / portTICK_PERIOD_MS);
        
        // Try to acquire mutex
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            return false;  // Timeout
        }
        
        // Check if queue is empty
        if (_queue.empty()) {
            xSemaphoreGive(_mutex);
            return false;  // Queue empty
        }
        
        // Peek at front item (copy, don't move)
        item = _queue.front();
        
        xSemaphoreGive(_mutex);
        return true;
    }

    /**
     * Get number of items currently in the queue
     */
    UBaseType_t getItemCount() const {
        if (!_mutex) return 0;
        
        // Need to take mutex to read size safely
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            UBaseType_t count = _queue.size();
            xSemaphoreGive(_mutex);
            return count;
        }
        return 0;
    }

    /**
     * Get number of free spaces in the queue
     */
    UBaseType_t getSpacesAvailable() const {
        if (_maxLength == 0) {
            return UINT32_MAX;  // Unlimited
        }
        return _maxLength - getItemCount();
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
        if (_maxLength == 0) return false;  // Unlimited
        return getItemCount() >= _maxLength;
    }

    /**
     * Reset the queue (remove all items)
     */
    void reset() {
        clear();
    }

    /**
     * Clear the queue (remove all items)
     */
    void clear() {
        if (!_mutex) return;
        
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            while (!_queue.empty()) {
                _queue.pop();
            }
            xSemaphoreGive(_mutex);
        }
    }

    /**
     * Get queue name
     */
    const char* getName() const { return _name; }

private:
    SemaphoreHandle_t _mutex;
    std::queue<T> _queue;
    UBaseType_t _maxLength;
    const char* _name;
};

