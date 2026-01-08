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
 * @file epon_hal_mock.h
 * @brief Mock EPON HAL for testing - Test trigger functions
 * 
 * Provides test-only functions to trigger callbacks and events.
 * These are NOT part of the real HAL interface.
 */

#ifndef EPON_HAL_MOCK_H
#define EPON_HAL_MOCK_H

#include "../../include/epon_hal.h"

/**
 * Test-only functions to trigger callbacks
 * These would NOT exist in the real HAL implementation
 */

/**
 * @brief Trigger ONU status change callback (test only)
 * @param status New ONU status
 */
void epon_hal_mock_trigger_status(epon_onu_status_t status);

/**
 * @brief Trigger alarm callback (test only)
 * @param type Alarm type (standard or vendor)
 * @param alarm_value Alarm enum value (cast to epon_hal_alarm_t or epon_vendor_alarm_t)
 * @param llid LLID value (use EPON_LLID_NOT_APPLICABLE if not applicable)
 * @param is_active True if alarm is active, false if cleared
 */
void epon_hal_mock_trigger_alarm(epon_alarm_type_t type, uint32_t alarm_value, uint16_t llid, bool is_active);

/**
 * @brief Trigger interface status change callback (test only)
 * @param interface_name Name of interface (e.g., "veip0")
 * @param status Interface link status
 */
void epon_hal_mock_trigger_interface_status(const char *interface_name, 
                                           epon_interface_link_status_t status);

/**
 * @brief Set mock link statistics for testing
 * @param stats Statistics to return in subsequent get_link_stats() calls
 */
void epon_hal_mock_set_link_stats(const epon_hal_link_stats_t *stats);

/**
 * @brief Set mock transceiver statistics for testing
 * @param stats Statistics to return in subsequent get_transceiver_stats() calls
 */
void epon_hal_mock_set_transceiver_stats(const epon_hal_transceiver_stats_t *stats);

#endif /* EPON_HAL_MOCK_H */
