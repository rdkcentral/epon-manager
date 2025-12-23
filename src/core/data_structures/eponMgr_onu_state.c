/*
 * EPON Manager - ONU State Management Implementation
 * Thread-safe ONU state tracking and coordination
 */

#include "eponMgr_onu_state.h"
#include <string.h>

int eponMgr_onu_state_init(eponMgr_onu_state_t *state, bool dpoe_supported)
{
    if (!state) {
        return -1;
    }

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

int eponMgr_onu_state_get_status(eponMgr_onu_state_t *state, 
                                  epon_onu_status_t *status)
{
    if (!state || !status) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);
    *status = state->current_status;
    pthread_mutex_unlock(&state->mutex);

    return 0;
}

bool eponMgr_onu_state_has_changed(eponMgr_onu_state_t *state)
{
    if (!state) {
        return false;
    }

    pthread_mutex_lock(&state->mutex);
    bool changed = (state->current_status != state->previous_status);
    pthread_mutex_unlock(&state->mutex);

    return changed;
}

int eponMgr_onu_state_update_olt_info(eponMgr_onu_state_t *state, 
                                       const epon_olt_info_t *olt_info)
{
    if (!state || !olt_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);
    
    memcpy(&state->olt_info, olt_info, sizeof(epon_olt_info_t));
    state->olt_info_valid = true;

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

int eponMgr_onu_state_get_olt_info(eponMgr_onu_state_t *state, 
                                    epon_olt_info_t *olt_info)
{
    if (!state || !olt_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);

    if (!state->olt_info_valid) {
        pthread_mutex_unlock(&state->mutex);
        return -1;
    }

    memcpy(olt_info, &state->olt_info, sizeof(epon_olt_info_t));

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

int eponMgr_onu_state_update_manufacturer_info(eponMgr_onu_state_t *state, 
                                                const epon_onu_manufacturer_info_t *mfr_info)
{
    if (!state || !mfr_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);
    
    memcpy(&state->manufacturer_info, mfr_info, sizeof(epon_onu_manufacturer_info_t));
    state->manufacturer_info_valid = true;

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

int eponMgr_onu_state_get_manufacturer_info(eponMgr_onu_state_t *state, 
                                             epon_onu_manufacturer_info_t *mfr_info)
{
    if (!state || !mfr_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);

    if (!state->manufacturer_info_valid) {
        pthread_mutex_unlock(&state->mutex);
        return -1;
    }

    memcpy(mfr_info, &state->manufacturer_info, sizeof(epon_onu_manufacturer_info_t));

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

int eponMgr_onu_state_update_link_info(eponMgr_onu_state_t *state, 
                                        const epon_hal_link_info_t *link_info)
{
    if (!state || !link_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);
    
    memcpy(&state->link_info, link_info, sizeof(epon_hal_link_info_t));
    state->link_info_valid = true;

    pthread_mutex_unlock(&state->mutex);

    return 0;
}

int eponMgr_onu_state_get_link_info(eponMgr_onu_state_t *state, 
                                     epon_hal_link_info_t *link_info)
{
    if (!state || !link_info) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);

    if (!state->link_info_valid) {
        pthread_mutex_unlock(&state->mutex);
        return -1;
    }

    memcpy(link_info, &state->link_info, sizeof(epon_hal_link_info_t));

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

bool eponMgr_onu_state_is_registered(eponMgr_onu_state_t *state)
{
    if (!state) {
        return false;
    }

    pthread_mutex_lock(&state->mutex);
    bool registered = (state->current_status == EPON_ONU_STATUS_REGISTRATION);
    pthread_mutex_unlock(&state->mutex);

    return registered;
}

bool eponMgr_onu_state_is_link_up(eponMgr_onu_state_t *state)
{
    if (!state) {
        return false;
    }

    pthread_mutex_lock(&state->mutex);
    // Consider ONU link as "up" when it's registered
    bool link_up = (state->current_status == EPON_ONU_STATUS_REGISTRATION);
    pthread_mutex_unlock(&state->mutex);

    return link_up;
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
