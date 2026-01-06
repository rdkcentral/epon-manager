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
 * @file eponMgr_psm.h
 * @brief EPON Manager PSM (Persistent Storage Manager) interface
 * 
 * Provides APIs to read/write persistent configuration using direct rbus calls to PSM.
 */

#ifndef EPONMGR_PSM_H
#define EPONMGR_PSM_H

#include <stdbool.h>
#include <stdint.h>
#include <rbus/rbus.h>

/* PSM parameter names for EPON Manager */
#define PSM_EPON_DPOE_ENABLE            "dmsb.eponmanager.DpoeEnable"
#define PSM_EPON_CACHE_TTL              "dmsb.eponmanager.CacheTtlSeconds"
#define PSM_EPON_STATS_POLLER_ENABLED   "dmsb.eponmanager.StatsPollerEnabled"
#define PSM_EPON_STATS_POLLER_INTERVAL  "dmsb.eponmanager.StatsPollerIntervalSeconds"

/**
 * @brief Initialize PSM connection
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_init(void);

/**
 * @brief Close PSM connection
 */
void eponMgr_psm_close(void);

/**
 * @brief Read string value from PSM
 * @param param PSM parameter name
 * @param value Buffer to store value
 * @param value_size Size of value buffer
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_string(const char *param, char *value, size_t value_size);

/**
 * @brief Read unsigned integer value from PSM
 * @param param PSM parameter name
 * @param value Pointer to store value
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_uint(const char *param, uint32_t *value);

/**
 * @brief Read boolean value from PSM
 * @param param PSM parameter name
 * @param value Pointer to store value
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_bool(const char *param, bool *value);

/**
 * @brief Write string value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_string(const char *param, const char *value);

/**
 * @brief Write unsigned integer value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_uint(const char *param, uint32_t value);

/**
 * @brief Write boolean value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_bool(const char *param, bool value);

#endif /* EPONMGR_PSM_H */
