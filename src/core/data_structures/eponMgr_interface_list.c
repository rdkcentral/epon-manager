/*
 * EPON Manager - Interface List Management
 * Uses HAL epon_interface_list_t directly
 */

#include "eponMgr_interface_list.h"
#include <string.h>

int eponMgr_interface_list_init(eponMgr_interface_list_t *list)
{
    if (!list) return -1;
    
    memset(&list->if_list, 0, sizeof(epon_interface_list_t));
    list->if_list.interface_count = 0;
    
    pthread_mutex_init(&list->mutex, NULL);
    return 0;
}

void eponMgr_interface_list_destroy(eponMgr_interface_list_t *list)
{
    if (!list) return;
    pthread_mutex_destroy(&list->mutex);
}

int eponMgr_interface_list_update(eponMgr_interface_list_t *list, const epon_onu_interface_info_t *intf_info)
{
    if (!list || !intf_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    // Find existing or add new
    for (uint32_t i = 0; i < list->if_list.interface_count; i++) {
        if (strncmp(list->if_list.interface[i].name, intf_info->name, EPON_HAL_INTERFACE_NAME_LEN) == 0) {
            // Update existing
            list->if_list.interface[i] = *intf_info;
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    // Add new if space available
    if (list->if_list.interface_count < EPON_HAL_MAX_INTERFACES) {
        list->if_list.interface[list->if_list.interface_count++] = *intf_info;
        pthread_mutex_unlock(&list->mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;  // List full
}

int eponMgr_interface_list_get(eponMgr_interface_list_t *list, const char *name, epon_onu_interface_info_t *intf_info)
{
    if (!list || !name || !intf_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    for (uint32_t i = 0; i < list->if_list.interface_count; i++) {
        if (strncmp(list->if_list.interface[i].name, name, EPON_HAL_INTERFACE_NAME_LEN) == 0) {
            *intf_info = list->if_list.interface[i];
            pthread_mutex_unlock(&list->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
    return -1;  // Not found
}

int eponMgr_interface_list_get_at(eponMgr_interface_list_t *list, uint32_t index, epon_onu_interface_info_t *intf_info)
{
    if (!list || !intf_info) return -1;
    
    pthread_mutex_lock(&list->mutex);
    
    if (index >= list->if_list.interface_count) {
        pthread_mutex_unlock(&list->mutex);
        return -1;
    }
    
    *intf_info = list->if_list.interface[index];
    pthread_mutex_unlock(&list->mutex);
    return 0;
}

uint32_t eponMgr_interface_list_count(eponMgr_interface_list_t *list)
{
    if (!list) return 0;
    
    pthread_mutex_lock(&list->mutex);
    uint32_t count = list->if_list.interface_count;
    pthread_mutex_unlock(&list->mutex);
    
    return count;
}

void eponMgr_interface_list_clear(eponMgr_interface_list_t *list)
{
    if (!list) return;
    
    pthread_mutex_lock(&list->mutex);
    list->if_list.interface_count = 0;
    memset(list->if_list.interface, 0, sizeof(list->if_list.interface));
    pthread_mutex_unlock(&list->mutex);
}
