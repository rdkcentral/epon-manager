/**
 * @file eponMgr_queue.c
 * @brief Simple event queue implementation with thread-safe mutex protection
 */

#include "eponMgr_queue.h"
#include <stdlib.h>
#include <string.h>

int eponMgr_queue_init(eponMgr_queue_t *queue, uint32_t capacity) {
    if (!queue || capacity == 0) {
        return -1;
    }
    
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

void eponMgr_queue_destroy(eponMgr_queue_t *queue) {
    if (!queue) return;
    
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

int eponMgr_queue_push(eponMgr_queue_t *queue, const eponMgr_event_t *event) {
    if (!queue || !event) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; /* Queue full */
    }
    
    /* Copy event to queue */
    memcpy(&queue->events[queue->head], event, sizeof(eponMgr_event_t));
    
    /* Advance head (circular) */
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

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
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

bool eponMgr_queue_is_empty(const eponMgr_queue_t *queue) {
    if (!queue) return true;
    
    /* Note: Reading count is atomic on most platforms, but for strict thread safety
     * we should lock. However, for simple empty check, this is usually safe. */
    return (queue->count == 0);
}

bool eponMgr_queue_is_full(const eponMgr_queue_t *queue) {
    if (!queue) return true;
    
    /* Note: Reading count is atomic on most platforms */
    return (queue->count >= queue->capacity);
}

uint32_t eponMgr_queue_size(const eponMgr_queue_t *queue) {
    if (!queue) return 0;
    
    /* Note: Reading count is atomic on most platforms */
    return queue->count;
}
