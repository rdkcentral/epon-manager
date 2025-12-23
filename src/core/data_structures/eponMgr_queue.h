/**
 * @file eponMgr_queue.h
 * @brief Simple event queue for EPON Manager
 * 
 * Circular buffer queue for events.
 * Thread-safe with pthread_mutex protection.
 */

#ifndef EPONMGR_QUEUE_H
#define EPONMGR_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../../../include/epon_hal.h"

/**
 * @brief Event types
 */
typedef enum {
    EPONMGR_EVENT_TYPE_ONU_STATUS = 0,
    EPONMGR_EVENT_TYPE_INTERFACE_STATUS,
    EPONMGR_EVENT_TYPE_ALARM
} eponMgr_event_type_t;

/**
 * @brief Event structure
 */
typedef struct {
    eponMgr_event_type_t type;
    
    union {
        /* ONU status event */
        struct {
            epon_onu_status_t status;
        } onu_status;
        
        /* Interface status event */
        struct {
            epon_onu_interface_info_t info;
        } interface_status;
        
        /* Alarm event */
        struct {
            epon_hal_alarm_t alarm;
            bool is_active;
        } alarm;
    } data;
} eponMgr_event_t;

/**
 * @brief Event queue structure (thread-safe)
 */
typedef struct {
    pthread_mutex_t mutex;       /**< Mutex for thread safety */
    eponMgr_event_t *events;     /**< Event array (circular buffer) */
    uint32_t capacity;           /**< Max capacity */
    uint32_t head;               /**< Write position */
    uint32_t tail;               /**< Read position */
    uint32_t count;              /**< Current count */
} eponMgr_queue_t;

/**
 * @brief Initialize event queue
 * @param queue Pointer to queue structure
 * @param capacity Maximum number of events
 * @return 0 on success, -1 on error
 */
int eponMgr_queue_init(eponMgr_queue_t *queue, uint32_t capacity);

/**
 * @brief Destroy event queue and free memory
 * @param queue Pointer to queue structure
 */
void eponMgr_queue_destroy(eponMgr_queue_t *queue);

/**
 * @brief Push event to queue
 * @param queue Pointer to queue structure
 * @param event Event to push
 * @return 0 on success, -1 if queue is full
 */
int eponMgr_queue_push(eponMgr_queue_t *queue, const eponMgr_event_t *event);

/**
 * @brief Pop event from queue
 * @param queue Pointer to queue structure
 * @param event Output buffer for event
 * @return 0 on success, -1 if queue is empty
 */
int eponMgr_queue_pop(eponMgr_queue_t *queue, eponMgr_event_t *event);

/**
 * @brief Check if queue is empty
 * @param queue Pointer to queue structure
 * @return true if empty, false otherwise
 */
bool eponMgr_queue_is_empty(const eponMgr_queue_t *queue);

/**
 * @brief Check if queue is full
 * @param queue Pointer to queue structure
 * @return true if full, false otherwise
 */
bool eponMgr_queue_is_full(const eponMgr_queue_t *queue);

/**
 * @brief Get current queue size
 * @param queue Pointer to queue structure
 * @return Number of events in queue
 */
uint32_t eponMgr_queue_size(const eponMgr_queue_t *queue);

#endif /* EPONMGR_QUEUE_H */
