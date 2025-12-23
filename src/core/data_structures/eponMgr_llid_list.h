/*
 * EPON Manager - LLID List Management
 * Uses HAL structs directly - no duplication
 */

#ifndef EPONMGR_LLID_LIST_H
#define EPONMGR_LLID_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "epon_hal.h"

#define EPONMGR_MAX_LLIDS 32  /**< Maximum LLIDs we can track */

/**
 * @brief Thread-safe LLID list manager
 * Uses epon_llid_list_t from HAL directly
 */
typedef struct {
    epon_llid_list_t llid_list;     /**< HAL LLID list structure */
    pthread_mutex_t mutex;           /**< Thread safety */
} eponMgr_llid_list_t;

int eponMgr_llid_list_init(eponMgr_llid_list_t *list, uint32_t max_llid_count);
void eponMgr_llid_list_destroy(eponMgr_llid_list_t *list);
int eponMgr_llid_list_update(eponMgr_llid_list_t *list, const epon_llid_info_t *llid_info);
int eponMgr_llid_list_remove(eponMgr_llid_list_t *list, uint16_t llid_value);
int eponMgr_llid_list_get(eponMgr_llid_list_t *list, uint16_t llid_value, epon_llid_info_t *llid_info);
int eponMgr_llid_list_get_at(eponMgr_llid_list_t *list, uint32_t index, epon_llid_info_t *llid_info);
uint32_t eponMgr_llid_list_count(eponMgr_llid_list_t *list);
void eponMgr_llid_list_clear(eponMgr_llid_list_t *list);

#endif /* EPONMGR_LLID_LIST_H */
