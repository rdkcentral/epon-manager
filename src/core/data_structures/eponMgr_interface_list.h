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

/*
 * EPON Manager - Interface List Management
 * Thread-safe interface list tracking for multi-interface EPON ONUs
 * Uses HAL structs directly to avoid memcpy overhead
 */

#ifndef EPONMGR_INTERFACE_LIST_H
#define EPONMGR_INTERFACE_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "epon_hal.h"

/**
 * @brief Thread-safe interface list manager
 * Uses epon_interface_list_t from HAL directly
 */
typedef struct {
    epon_interface_list_t if_list;  /**< HAL interface list structure */
    pthread_mutex_t mutex;           /**< Mutex for thread-safe access */
} eponMgr_interface_list_t;

/**
 * @brief Initialize interface list
 * @return 0 on success, -1 on error
 */
int eponMgr_interface_list_init(eponMgr_interface_list_t *list);

/**
 * @brief Destroy interface list
 */
void eponMgr_interface_list_destroy(eponMgr_interface_list_t *list);

/**
 * @brief Update interface status (adds if new, updates if exists)
 * @param list Pointer to interface list
 * @param intf_info Pointer to HAL interface info structure
 * @return 0 on success, -1 on error
 */
int eponMgr_interface_list_update(eponMgr_interface_list_t *list, 
                                   const epon_onu_interface_info_t *intf_info);

/**
 * @brief Get interface by name
 * @param list Pointer to interface list
 * @param name Interface name to search
 * @param intf_info Pointer to store result (HAL struct)
 * @return 0 on success, -1 if not found
 */
int eponMgr_interface_list_get(eponMgr_interface_list_t *list, 
                                const char *name,
                                epon_onu_interface_info_t *intf_info);

/**
 * @brief Get interface by index
 * @param list Pointer to interface list
 * @param index Index (0-based)
 * @param intf_info Pointer to store result (HAL struct)
 * @return 0 on success, -1 if invalid index
 */
int eponMgr_interface_list_get_at(eponMgr_interface_list_t *list,
                                   uint32_t index,
                                   epon_onu_interface_info_t *intf_info);

/**
 * @brief Get count of interfaces
 * @return Number of valid interfaces
 */
uint32_t eponMgr_interface_list_count(eponMgr_interface_list_t *list);

/**
 * @brief Clear all interfaces
 */
void eponMgr_interface_list_clear(eponMgr_interface_list_t *list);

#endif /* EPONMGR_INTERFACE_LIST_H */
