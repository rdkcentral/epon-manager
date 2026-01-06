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
 * EPON Manager - CPE MAC Address List Management
 * Uses HAL structs directly - no duplication
 */

#ifndef EPONMGR_CPE_LIST_H
#define EPONMGR_CPE_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "epon_hal.h"

#define EPONMGR_MAX_CPE_ENTRIES 256  /**< Maximum CPE entries */

/**
 * @brief Thread-safe CPE list manager
 * Uses dpoe_cpe_mac_table_t from HAL directly
 */
typedef struct {
    dpoe_cpe_mac_table_t cpe_table;  /**< HAL CPE MAC table structure */
    pthread_mutex_t mutex;            /**< Thread safety */
} eponMgr_cpe_list_t;

int eponMgr_cpe_list_init(eponMgr_cpe_list_t *list, uint32_t max_cpe);
void eponMgr_cpe_list_destroy(eponMgr_cpe_list_t *list);
int eponMgr_cpe_list_update(eponMgr_cpe_list_t *list, const dpoe_cpe_mac_entry_t *cpe_entry);
int eponMgr_cpe_list_remove(eponMgr_cpe_list_t *list, const uint8_t mac_address[EPON_HAL_MAC_ADDR_LEN]);
int eponMgr_cpe_list_get(eponMgr_cpe_list_t *list, const uint8_t mac_address[EPON_HAL_MAC_ADDR_LEN], dpoe_cpe_mac_entry_t *cpe_entry);
int eponMgr_cpe_list_get_at(eponMgr_cpe_list_t *list, uint32_t index, dpoe_cpe_mac_entry_t *cpe_entry);
uint32_t eponMgr_cpe_list_count(eponMgr_cpe_list_t *list);
void eponMgr_cpe_list_clear(eponMgr_cpe_list_t *list);
void eponMgr_cpe_list_clear_dynamic(eponMgr_cpe_list_t *list);

#endif /* EPONMGR_CPE_LIST_H */
