/*
 * EPON Manager - LLID List Management
 * Uses HAL epon_llid_list_t directly with dynamic memory
 */

#include "eponMgr_llid_list.h"
#include "eponMgr_tr181.h"
#include "eponMgr_logger.h"
#include <string.h>
#include <stdlib.h>

int eponMgr_llid_list_init(eponMgr_llid_list_t *list, uint32_t max_llid_count)
{
    if (!list) return -1;
    
    EPONMGR_LOG_INFO("Initializing LLID list with max count: %u\n", max_llid_count);
    
    memset(&list->llid_list, 0, sizeof(epon_llid_list_t));
    list->llid_list.max_llid_count = max_llid_count;
    list->llid_list.llid_count = 0;
    list->llid_list.llid_list = NULL;  // Allocate on first use
    
    pthread_mutex_init(&list->mutex, NULL);
    return 0;
}

void eponMgr_llid_list_destroy(eponMgr_llid_list_t *list)
{
    if (!list) return;
    
    EPONMGR_LOG_INFO("Destroying LLID list\n");
    
    pthread_mutex_lock(&list->mutex);
    if (list->llid_list.llid_list) {
        free(list->llid_list.llid_list);
        list->llid_list.llid_list = NULL;
    }
    pthread_mutex_unlock(&list->mutex);
    
    pthread_mutex_destroy(&list->mutex);
}

int eponMgr_llid_list_update(eponMgr_llid_list_t *list, const epon_llid_info_t *llid_info)
{
    if (!list || !llid_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    // Find existing LLID
    for (uint32_t i = 0; i < list->llid_list.llid_count; i++) {
        if (list->llid_list.llid_list[i].llid_value == llid_info->llid_value) {
            // Update existing
            list->llid_list.llid_list[i] = *llid_info;
            pthread_mutex_unlock(&list->mutex);
            return 0;  // Existing entry updated, no change in count
        }
    }
    
    // Add new LLID - need to realloc
    if (list->llid_list.llid_count >= list->llid_list.max_llid_count) {
        pthread_mutex_unlock(&list->mutex);
        return -1;  // Max capacity reached
    }
    
    epon_llid_info_t *new_list = realloc(list->llid_list.llid_list, 
                                         (list->llid_list.llid_count + 1) * sizeof(epon_llid_info_t));
    if (!new_list) {
        pthread_mutex_unlock(&list->mutex);
        return -1;
    }
    
    list->llid_list.llid_list = new_list;
    list->llid_list.llid_list[list->llid_list.llid_count++] = *llid_info;
    
    pthread_mutex_unlock(&list->mutex);
    return 1;  // New LLID added
}

int eponMgr_llid_list_remove(eponMgr_llid_list_t *list, uint16_t llid_value)
{
    if (!list) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    // Find and remove LLID
    for (uint32_t i = 0; i < list->llid_list.llid_count; i++) {
        if (list->llid_list.llid_list[i].llid_value == llid_value) {
            // Shift remaining elements
            for (uint32_t j = i; j < list->llid_list.llid_count - 1; j++) {
                list->llid_list.llid_list[j] = list->llid_list.llid_list[j + 1];
            }
            list->llid_list.llid_count--;
            
            // Shrink allocation if needed
            if (list->llid_list.llid_count > 0) {
                epon_llid_info_t *new_list = realloc(list->llid_list.llid_list, 
                                                     list->llid_list.llid_count * sizeof(epon_llid_info_t));
                if (new_list) {
                    list->llid_list.llid_list = new_list;
                }
            } else {
                free(list->llid_list.llid_list);
                list->llid_list.llid_list = NULL;
            }
            
            pthread_mutex_unlock(&list->mutex);
            
            /* Sync TR-181 table registrations after removal */
            eponMgr_tr181_sync_llid_table();
            
            return 0;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;  // Not found
}

int eponMgr_llid_list_get(eponMgr_llid_list_t *list, uint16_t llid_value, epon_llid_info_t *llid_info)
{
    if (!list || !llid_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    for (uint32_t i = 0; i < list->llid_list.llid_count; i++) {
        if (list->llid_list.llid_list[i].llid_value == llid_value) {
            *llid_info = list->llid_list.llid_list[i];
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;
}

int eponMgr_llid_list_get_at(eponMgr_llid_list_t *list, uint32_t index, epon_llid_info_t *llid_info)
{
    if (!list || !llid_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    if (index >= list->llid_list.llid_count) {
        pthread_mutex_unlock(&list->mutex);
        return -1;
    }
    
    *llid_info = list->llid_list.llid_list[index];
    pthread_mutex_unlock(&list->mutex);
    return 0;
}

uint32_t eponMgr_llid_list_count(eponMgr_llid_list_t *list)
{
    if (!list) return 0;
    
    pthread_mutex_lock(&list->mutex);
    uint32_t count = list->llid_list.llid_count;
    pthread_mutex_unlock(&list->mutex);
    
    return count;
}

void eponMgr_llid_list_clear(eponMgr_llid_list_t *list)
{
    if (!list) return;
    
    pthread_mutex_lock(&list->mutex);
    
    if (list->llid_list.llid_list) {
        free(list->llid_list.llid_list);
        list->llid_list.llid_list = NULL;
    }
    list->llid_list.llid_count = 0;
    
    pthread_mutex_unlock(&list->mutex);
}
