/**
 * @file eponMgr_psm.c
 * @brief EPON Manager PSM (Persistent Storage Manager) implementation
 * 
 * Direct rbus method calls to PSM without using ccsp_psm_helper dependency
 */

#include "eponMgr_psm.h"
#include "eponMgr_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rbus/rbus.h>

/* Global rbus handle */
static rbusHandle_t g_rbus_handle = NULL;

int eponMgr_psm_init(void) {
    // rbus handle should be set from main controller
    EPONMGR_LOG_INFO("PSM interface initialized\n");
    return 0;
}

void eponMgr_psm_close(void) {
    EPONMGR_LOG_INFO("PSM interface closed\n");
}

/**
 * @brief Set the rbus handle for PSM operations
 * This should be called after rbus initialization in the controller
 */
void eponMgr_psm_set_rbus_handle(rbusHandle_t handle) {
    g_rbus_handle = handle;
}

int eponMgr_psm_get_string(const char *param, char *value, size_t value_size) {
    if (!param || !value || value_size == 0) {
        return -1;
    }
    
    if (!g_rbus_handle) {
        EPONMGR_LOG_ERROR("PSM: rbus handle not initialized\n");
        return -1;
    }
    
    /* Create input parameters */
    rbusObject_t inParams = NULL, outParams = NULL;
    rbusObject_Init(&inParams, NULL);
    
    rbusProperty_t prop;
    rbusValue_t rbusVal;
    rbusValue_Init(&rbusVal);
    rbusValue_SetString(rbusVal, param);
    rbusProperty_Init(&prop, "param0", rbusVal);
    rbusValue_Release(rbusVal);
    rbusObject_SetProperties(inParams, prop);
    rbusProperty_Release(prop);
    
    /* Invoke GetPSMRecordValue method */
    rbusError_t rc = rbusMethod_Invoke(g_rbus_handle, "GetPSMRecordValue()", inParams, &outParams);
    
    if (inParams) {
        rbusObject_Release(inParams);
    }
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_DEBUG("PSM get failed for %s: rc=%d\n", param, rc);
        if (outParams) {
            rbusObject_Release(outParams);
        }
        return -1;
    }
    
    /* Parse output */
    if (outParams) {
        rbusProperty_t outProp = rbusObject_GetProperties(outParams);
        if (outProp) {
            rbusValue_t val = rbusProperty_GetValue(outProp);
            if (val) {
                const char *str_val = rbusValue_GetString(val, NULL);
                if (str_val) {
                    strncpy(value, str_val, value_size - 1);
                    value[value_size - 1] = '\0';
                    EPONMGR_LOG_DEBUG("PSM get: %s = %s\n", param, value);
                    rbusObject_Release(outParams);
                    return 0;
                }
            }
        }
        rbusObject_Release(outParams);
    }
    
    EPONMGR_LOG_ERROR("PSM get failed: no value returned for %s\n", param);
    return -1;
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
    
    if (!g_rbus_handle) {
        EPONMGR_LOG_ERROR("PSM: rbus handle not initialized\n");
        return -1;
    }
    
    /* Create input parameters */
    rbusObject_t inParams = NULL, outParams = NULL;
    rbusObject_Init(&inParams, NULL);
    
    /* Create property for parameter */
    rbusObject_t paramObj = NULL;
    rbusObject_Init(&paramObj, param);
    rbusObject_SetPropertyString(paramObj, "value", value);
    rbusObject_SetPropertyString(paramObj, "type", "astr");
    
    rbusProperty_t prop;
    rbusValue_t rbusVal;
    rbusValue_Init(&rbusVal);
    rbusValue_SetFromObject(rbusVal, paramObj);
    rbusProperty_Init(&prop, "param0", rbusVal);
    rbusValue_Release(rbusVal);
    rbusObject_Release(paramObj);
    
    rbusObject_SetProperties(inParams, prop);
    rbusProperty_Release(prop);
    
    /* Invoke SetPSMRecordValue method */
    rbusError_t rc = rbusMethod_Invoke(g_rbus_handle, "SetPSMRecordValue()", inParams, &outParams);
    
    if (inParams) {
        rbusObject_Release(inParams);
    }
    
    if (outParams) {
        rbusObject_Release(outParams);
    }
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("PSM set failed for %s: rc=%d\n", param, rc);
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

