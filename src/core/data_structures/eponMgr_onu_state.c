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
 * EPON Manager - ONU State Management Implementation
 * Thread-safe ONU state tracking and coordination
 */

#include "eponMgr_onu_state.h"
#include "eponMgr_logger.h"
#include <string.h>

int eponMgr_onu_state_init(eponMgr_onu_state_t *state, bool dpoe_supported)
{
    if (!state) {
        return -1;
    }

    EPONMGR_LOG_INFO("Initializing ONU state (DPoE %s)\n", dpoe_supported ? "enabled" : "disabled");
    
    memset(state, 0, sizeof(eponMgr_onu_state_t));
    
    state->current_status = EPON_ONU_STATUS_LOS;
    state->previous_status = EPON_ONU_STATUS_LOS;
    state->dpoe_supported = dpoe_supported;
    state->hal_initialized = false;
    
    if (pthread_mutex_init(&state->mutex, NULL) != 0) {
        return -1;
    }

    return 0;
}

void eponMgr_onu_state_destroy(eponMgr_onu_state_t *state)
{
    if (!state) {
        return;
    }

    EPONMGR_LOG_INFO("Destroying ONU state\n");
    
    pthread_mutex_destroy(&state->mutex);
    memset(state, 0, sizeof(eponMgr_onu_state_t));
}

int eponMgr_onu_state_update_status(eponMgr_onu_state_t *state, 
                                     epon_onu_status_t new_status)
{
    if (!state) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);

    state->previous_status = state->current_status;
    state->current_status = new_status;
    state->last_status_change = time(NULL);

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

void eponMgr_onu_state_invalidate_all(eponMgr_onu_state_t *state)
{
    if (!state) {
        return;
    }

    pthread_mutex_lock(&state->mutex);
    
    state->olt_info_valid = false;
    state->manufacturer_info_valid = false;
    state->link_info_valid = false;

    pthread_mutex_unlock(&state->mutex);
}

void eponMgr_onu_state_set_hal_initialized(eponMgr_onu_state_t *state)
{
    if (!state) {
        return;
    }

    pthread_mutex_lock(&state->mutex);
    state->hal_initialized = true;
    pthread_mutex_unlock(&state->mutex);
}

bool eponMgr_onu_state_is_hal_initialized(eponMgr_onu_state_t *state)
{
    if (!state) {
        return false;
    }

    pthread_mutex_lock(&state->mutex);
    bool initialized = state->hal_initialized;
    pthread_mutex_unlock(&state->mutex);

    return initialized;
}
