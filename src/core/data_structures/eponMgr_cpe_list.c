/*
 * EPON Manager - CPE List Management
 * Uses HAL dpoe_cpe_mac_table_t directly with dynamic memory
 */

#include "eponMgr_cpe_list.h"
#include <string.h>
#include <stdlib.h>

static int mac_equal(const uint8_t *mac1, const uint8_t *mac2)
{
    return memcmp(mac1, mac2, 6) == 0;
}

int eponMgr_cpe_list_init(eponMgr_cpe_list_t *list, uint32_t max_cpe)
{
    if (!list) return -1;
    
    memset(&list->cpe_table, 0, sizeof(dpoe_cpe_mac_table_t));
    list->cpe_table.max_cpe = max_cpe;
    list->cpe_table.static_cpe_count = 0;
    list->cpe_table.dynamic_cpe_count = 0;
    list->cpe_table.cpe_list = NULL;  // Allocate on first use
    
    pthread_mutex_init(&list->mutex, NULL);
    return 0;
}

void eponMgr_cpe_list_destroy(eponMgr_cpe_list_t *list)
{
    if (!list) return;
    
    pthread_mutex_lock(&list->mutex);
    if (list->cpe_table.cpe_list) {
        free(list->cpe_table.cpe_list);
        list->cpe_table.cpe_list = NULL;
    }
    pthread_mutex_unlock(&list->mutex);
    
    pthread_mutex_destroy(&list->mutex);
}

int eponMgr_cpe_list_update(eponMgr_cpe_list_t *list, const dpoe_cpe_mac_entry_t *cpe_entry)
{
    if (!list || !cpe_entry) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    uint32_t total_count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    
    // Find existing CPE
    for (uint32_t i = 0; i < total_count; i++) {
        if (mac_equal(list->cpe_table.cpe_list[i].mac_address, cpe_entry->mac_address)) {
            // Update existing - adjust counts if type changed
            if (list->cpe_table.cpe_list[i].type != cpe_entry->type) {
                if (list->cpe_table.cpe_list[i].type == DPOE_CPE_MAC_STATIC) {
                    list->cpe_table.static_cpe_count--;
                    list->cpe_table.dynamic_cpe_count++;
                } else {
                    list->cpe_table.dynamic_cpe_count--;
                    list->cpe_table.static_cpe_count++;
                }
            }
            list->cpe_table.cpe_list[i] = *cpe_entry;
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    // Add new CPE
    if (total_count >= list->cpe_table.max_cpe) {
        pthread_mutex_unlock(&list->mutex);
        return -1;  // Max capacity reached
    }
    
    dpoe_cpe_mac_entry_t *new_list = realloc(list->cpe_table.cpe_list, 
                                              (total_count + 1) * sizeof(dpoe_cpe_mac_entry_t));
    if (!new_list) {
        pthread_mutex_unlock(&list->mutex);
        return -1;
    }
    
    list->cpe_table.cpe_list = new_list;
    list->cpe_table.cpe_list[total_count] = *cpe_entry;
    
    if (cpe_entry->type == DPOE_CPE_MAC_STATIC) {
        list->cpe_table.static_cpe_count++;
    } else {
        list->cpe_table.dynamic_cpe_count++;
    }
    
    pthread_mutex_unlock(&list->mutex);
    return 0;
}

int eponMgr_cpe_list_remove(eponMgr_cpe_list_t *list, const uint8_t *mac_address)
{
    if (!list || !mac_address) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    uint32_t total_count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    
    // Find and remove CPE
    for (uint32_t i = 0; i < total_count; i++) {
        if (mac_equal(list->cpe_table.cpe_list[i].mac_address, mac_address)) {
            // Adjust count
            if (list->cpe_table.cpe_list[i].type == DPOE_CPE_MAC_STATIC) {
                list->cpe_table.static_cpe_count--;
            } else {
                list->cpe_table.dynamic_cpe_count--;
            }
            
            // Shift remaining elements
            for (uint32_t j = i; j < total_count - 1; j++) {
                list->cpe_table.cpe_list[j] = list->cpe_table.cpe_list[j + 1];
            }
            
            total_count--;
            
            // Shrink allocation
            if (total_count > 0) {
                dpoe_cpe_mac_entry_t *new_list = realloc(list->cpe_table.cpe_list, 
                                                         total_count * sizeof(dpoe_cpe_mac_entry_t));
                if (new_list) {
                    list->cpe_table.cpe_list = new_list;
                }
            } else {
                free(list->cpe_table.cpe_list);
                list->cpe_table.cpe_list = NULL;
            }
            
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;  // Not found
}

int eponMgr_cpe_list_get(eponMgr_cpe_list_t *list, const uint8_t *mac_address, dpoe_cpe_mac_entry_t *cpe_entry)
{
    if (!list || !mac_address || !cpe_entry) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    uint32_t total_count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    
    for (uint32_t i = 0; i < total_count; i++) {
        if (mac_equal(list->cpe_table.cpe_list[i].mac_address, mac_address)) {
            *cpe_entry = list->cpe_table.cpe_list[i];
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;
}

int eponMgr_cpe_list_get_at(eponMgr_cpe_list_t *list, uint32_t index, dpoe_cpe_mac_entry_t *cpe_entry)
{
    if (!list || !cpe_entry) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    uint32_t total_count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    
    if (index >= total_count) {
        pthread_mutex_unlock(&list->mutex);
        return -1;
    }
    
    *cpe_entry = list->cpe_table.cpe_list[index];
    pthread_mutex_unlock(&list->mutex);
    return 0;
}

uint32_t eponMgr_cpe_list_count(eponMgr_cpe_list_t *list)
{
    if (!list) return 0;
    
    pthread_mutex_lock(&list->mutex);
    uint32_t count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    pthread_mutex_unlock(&list->mutex);
    
    return count;
}

void eponMgr_cpe_list_clear(eponMgr_cpe_list_t *list)
{
    if (!list) return;
    
    pthread_mutex_lock(&list->mutex);
    
    if (list->cpe_table.cpe_list) {
        free(list->cpe_table.cpe_list);
        list->cpe_table.cpe_list = NULL;
    }
    list->cpe_table.static_cpe_count = 0;
    list->cpe_table.dynamic_cpe_count = 0;
    
    pthread_mutex_unlock(&list->mutex);
}

void eponMgr_cpe_list_clear_dynamic(eponMgr_cpe_list_t *list)
{
    if (!list) return;
    
    pthread_mutex_lock(&list->mutex);
    
    uint32_t total_count = list->cpe_table.static_cpe_count + list->cpe_table.dynamic_cpe_count;
    uint32_t new_count = 0;
    
    // Keep only static entries
    for (uint32_t i = 0; i < total_count; i++) {
        if (list->cpe_table.cpe_list[i].type == DPOE_CPE_MAC_STATIC) {
            if (new_count != i) {
                list->cpe_table.cpe_list[new_count] = list->cpe_table.cpe_list[i];
            }
            new_count++;
        }
    }
    
    list->cpe_table.dynamic_cpe_count = 0;
    
    // Shrink allocation
    if (new_count > 0 && new_count < total_count) {
        dpoe_cpe_mac_entry_t *new_list = realloc(list->cpe_table.cpe_list, 
                                                 new_count * sizeof(dpoe_cpe_mac_entry_t));
        if (new_list) {
            list->cpe_table.cpe_list = new_list;
        }
    } else if (new_count == 0) {
        free(list->cpe_table.cpe_list);
        list->cpe_table.cpe_list = NULL;
    }
    
    pthread_mutex_unlock(&list->mutex);
}
