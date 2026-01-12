/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file eponMgr_queue.c
 * @brief Simple event queue implementation with thread-safe mutex protection
 */

#include "eponMgr_queue.h"
#include "eponMgr_logger.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize event queue
 * 
 * Creates a circular buffer event queue with the specified capacity and initializes
 * the mutex for thread-safe access. Memory is allocated for the event array.
 * 
 * @param queue Pointer to queue structure
 * @param capacity Maximum number of events
 * @return 0 on success, -1 on error
 * 
 * @note Caller must call eponMgr_queue_destroy() to free allocated memory
 * @note Mutex is locked/unlocked internally by push/pop operations
 * @note All fields are zero-initialized before allocation
 */
int eponMgr_queue_init(eponMgr_queue_t *queue, uint32_t capacity) {
    if (!queue || capacity == 0) {
        return -1;
    }
    
    EPONMGR_LOG_INFO("Initializing event queue with capacity: %u\n", capacity);
    
    memset(queue, 0, sizeof(eponMgr_queue_t));
    
    queue->events = (eponMgr_event_t *)malloc(capacity * sizeof(eponMgr_event_t));
    if (!queue->events) {
        return -1;
    }
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->events);
        return -1;
    }
    
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    return 0;
}

/**
 * @brief Destroy event queue and free memory
 * 
 * Frees the event array memory and destroys the mutex. All queued events are lost.
 * 
 * @param queue Pointer to queue structure
 * 
 * @note Caller must ensure no threads are using the queue before calling
 * @note Allocated memory is freed and pointer is set to NULL
 * @note Safe to call with NULL pointer
 */
void eponMgr_queue_destroy(eponMgr_queue_t *queue) {
    if (!queue) return;
    
    EPONMGR_LOG_INFO("Destroying event queue (capacity: %u)\n", queue->capacity);
    
    pthread_mutex_destroy(&queue->mutex);
    
    if (queue->events) {
        free(queue->events);
        queue->events = NULL;
    }
    
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/**
 * @brief Push event to queue
 * 
 * Adds an event to the queue using circular buffer logic. If queue is full,
 * the event is discarded and error is returned.
 * 
 * @param queue Pointer to queue structure
 * @param event Event to push
 * @return 0 on success, -1 if queue is full or on error
 * 
 * @note Thread-safe - mutex is locked/unlocked internally
 * @note Event is deep-copied into queue
 * @note If queue is full, oldest events must be popped first
 */
int eponMgr_queue_push(eponMgr_queue_t *queue, const eponMgr_event_t *event) {
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        EPONMGR_LOG_INFO("Event queue is full, discarding event\n");
        return -1; /* Queue full */
    }
    
    /* Copy event to queue */
    memcpy(&queue->events[queue->head], event, sizeof(eponMgr_event_t));
    
    /* Advance head (circular) */
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count++;
    
    EPONMGR_LOG_INFO("Event pushed to queue (count: %u/%u)\n", queue->count, queue->capacity);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

/**
 * @brief Pop event from queue
 * 
 * Retrieves and removes the oldest event from the queue using circular buffer logic.
 * Returns error if queue is empty.
 * 
 * @param queue Pointer to queue structure
 * @param event Output buffer for event
 * @return 0 on success, -1 if queue is empty or on error
 * 
 * @note Thread-safe - mutex is locked/unlocked internally
 * @note Event is copied to caller's buffer
 * @note Caller must provide valid event buffer
 */
int eponMgr_queue_pop(eponMgr_queue_t *queue, eponMgr_event_t *event) {
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; /* Queue empty */
    }
    
    /* Copy event from queue */
    memcpy(event, &queue->events[queue->tail], sizeof(eponMgr_event_t));
    
    /* Advance tail (circular) */
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count--;
    
    EPONMGR_LOG_INFO("Event popped from queue (count: %u/%u)\n", queue->count, queue->capacity);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

/**
 * @brief Check if queue is empty
 * 
 * Thread-safe check of queue empty status.
 * 
 * @param queue Pointer to queue structure
 * @return true if empty, false otherwise
 * 
 * @note Thread-safe - mutex is locked/unlocked internally
 * @note Returns true if queue pointer is NULL
 */
bool eponMgr_queue_is_empty(const eponMgr_queue_t *queue) {
    if (!queue) return true;
    
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    bool empty = (queue->count == 0);
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
    return empty;
}

/**
 * @brief Check if queue is full
 * 
 * Thread-safe check of queue full status.
 * 
 * @param queue Pointer to queue structure
 * @return true if full, false otherwise
 * 
 * @note Thread-safe - mutex is locked/unlocked internally
 * @note Returns true if queue pointer is NULL
 */
bool eponMgr_queue_is_full(const eponMgr_queue_t *queue) {
    if (!queue) return true;
    
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    bool full = (queue->count >= queue->capacity);
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
    return full;
}

/**
 * @brief Get current queue size
 * 
 * Thread-safe retrieval of the number of events currently in the queue.
 * 
 * @param queue Pointer to queue structure
 * @return Number of events in queue
 * 
 * @note Thread-safe - mutex is locked/unlocked internally
 * @note Returns 0 if queue pointer is NULL
 */
uint32_t eponMgr_queue_size(const eponMgr_queue_t *queue) {
    if (!queue) return 0;
    
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    uint32_t size = queue->count;
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
    return size;
}
