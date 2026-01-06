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
