/**
 * @file eponMgr_psm.c
 * @brief EPON Manager PSM (Persistent Storage Manager) implementation
 */

#include "eponMgr_psm.h"
#include "eponMgr_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ccsp_psm_helper.h>

/* PSM component name */
#define EPON_PSM_COMPONENT_NAME    "epon_manager"

/* Global PSM handle */
static void *g_psm_bus_handle = NULL;

int eponMgr_psm_init(void) {
    // PSM connection is handled by CCSP framework
    // We just need to get the bus handle from the system
    // This is typically initialized by the CCSP subsystem
    EPONMGR_LOG_INFO("PSM interface initialized\n");
    return 0;
}

void eponMgr_psm_close(void) {
    // PSM cleanup handled by CCSP framework
    EPONMGR_LOG_INFO("PSM interface closed\n");
}

int eponMgr_psm_get_string(const char *param, char *value, size_t value_size) {
    if (!param || !value || value_size == 0) {
        return -1;
    }
    
    char *psm_value = NULL;
    int ret = PSM_Get_Record_Value2(g_psm_bus_handle, 
                                    CCSP_SUBSYS, 
                                    param, 
                                    NULL, 
                                    &psm_value);
    
    if (ret != CCSP_SUCCESS || !psm_value) {
        EPONMGR_LOG_DEBUG("PSM get failed for %s: ret=%d\n", param, ret);
        return -1;
    }
    
    strncpy(value, psm_value, value_size - 1);
    value[value_size - 1] = '\0';
    
    // Free PSM allocated memory
    if (psm_value) {
        ((CCSP_MESSAGE_BUS_INFO *)g_psm_bus_handle)->freefunc(psm_value);
    }
    
    EPONMGR_LOG_DEBUG("PSM get: %s = %s\n", param, value);
    return 0;
}

int eponMgr_psm_get_uint(const char *param, uint32_t *value) {
    char str_value[32];
    
    if (!param || !value) {
        return -1;
    }
    
    if (eponMgr_psm_get_string(param, str_value, sizeof(str_value)) != 0) {
        return -1;
    }
    
    *value = (uint32_t)atoi(str_value);
    return 0;
}

int eponMgr_psm_get_bool(const char *param, bool *value) {
    char str_value[16];
    
    if (!param || !value) {
        return -1;
    }
    
    if (eponMgr_psm_get_string(param, str_value, sizeof(str_value)) != 0) {
        return -1;
    }
    
    *value = (strcmp(str_value, "TRUE") == 0 || strcmp(str_value, "true") == 0 || strcmp(str_value, "1") == 0);
    return 0;
}

int eponMgr_psm_set_string(const char *param, const char *value) {
    if (!param || !value) {
        return -1;
    }
    
    int ret = PSM_Set_Record_Value2(g_psm_bus_handle,
                                    CCSP_SUBSYS,
                                    param,
                                    ccsp_string,
                                    (char *)value);
    
    if (ret != CCSP_SUCCESS) {
        EPONMGR_LOG_ERROR("PSM set failed for %s: ret=%d\n", param, ret);
        return -1;
    }
    
    EPONMGR_LOG_DEBUG("PSM set: %s = %s\n", param, value);
    return 0;
}

int eponMgr_psm_set_uint(const char *param, uint32_t value) {
    char str_value[32];
    snprintf(str_value, sizeof(str_value), "%u", value);
    return eponMgr_psm_set_string(param, str_value);
}

int eponMgr_psm_set_bool(const char *param, bool value) {
    return eponMgr_psm_set_string(param, value ? "TRUE" : "FALSE");
}
