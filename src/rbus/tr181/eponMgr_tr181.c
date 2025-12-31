/**
 * @file eponMgr_tr181.c
 * @brief TR-181 Parameter Handler Implementation with Real HAL Integration
 * 
 * Implements GET/SET handlers for Device.Optical.Interface.{i} parameters.
 * Maps TR-181 parameters to EPON HAL wrapper APIs with caching.
 * 
 * Includes dynamic table support for:
 * - LLID.{i} table (Phase 7.1)
 * - DPoE.CPE.{i} table (Phase 7.2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "eponMgr_tr181.h"
#include "eponMgr_logger.h"
#include "eponMgr_controller.h"
#include "eponMgr_persistence.h"

#include <rbus/rbus.h>

/* TR-181 Base Path */
#define TR181_BASE_PATH "Device.Optical.Interface.1"
#define MAX_LLID_INSTANCES 64
#define MAX_CPE_INSTANCES 256

/* Dynamic LLID table tracking */
typedef struct {
    uint32_t instance;  /* 1-based instance number */
    bool registered;
} llid_instance_t;

/* Dynamic CPE table tracking */
typedef struct {
    uint32_t instance;  /* 1-based instance number */
    bool registered;
} cpe_instance_t;

/* Static state */
static rbusHandle_t g_rbus_handle = NULL;
static int g_param_count = 0;
static llid_instance_t g_llid_instances[MAX_LLID_INSTANCES] = {{0}};
static pthread_mutex_t g_llid_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static cpe_instance_t g_cpe_instances[MAX_CPE_INSTANCES] = {{0}};
static pthread_mutex_t g_cpe_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Forward declarations for handlers */
static rbusError_t base_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t base_param_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);
static rbusError_t optical_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t stats_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t epon_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t manufacturer_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t olt_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t llid_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t cpe_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);

/**
 * @brief TR-181 Parameter Registration Table
 * 
 * Each entry defines a TR-181 parameter with its RBUS handlers.
 * Parameters are organized by category for easier maintenance.
 */
static rbusDataElement_t g_tr181_params[] = {
    /* Base Interface Parameters (7 parameters) */
    {TR181_BASE_PATH ".Enable", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, base_param_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Status", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Alias", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, base_param_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Name", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LowerLayers", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Upstream", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Optical Parameters (6 parameters) */
    {TR181_BASE_PATH ".OpticalSignalLevel", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LowerOpticalThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".UpperOpticalThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".TransmitOpticalLevel", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LowerTransmitPowerThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".UpperTransmitPowerThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Standard Stats (15 parameters) */
    {TR181_BASE_PATH ".Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.UnknownProtoPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* X_RDK Stats (5 parameters) */
    {TR181_BASE_PATH ".Stats.X_RDK_FECCorrected", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_FECUncorrectable", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_BER", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_RangingResyncs", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_MACResets", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* X_RDK_Transceiver (3 parameters) */
    {TR181_BASE_PATH ".X_RDK_Transceiver.Temperature", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.SupplyVoltage", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.BiasCurrent", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* X_RDK_EPON (5 parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.OperationalMode", RBUS_ELEMENT_TYPE_PROPERTY, {epon_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.EncryptionMode", RBUS_ELEMENT_TYPE_PROPERTY, {epon_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.ONUStatus", RBUS_ELEMENT_TYPE_PROPERTY, {epon_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPoESupported", RBUS_ELEMENT_TYPE_PROPERTY, {epon_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.MaxLLIDSupported", RBUS_ELEMENT_TYPE_PROPERTY, {epon_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Manufacturer Info (6 parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.Manufacturer", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.ModelNumber", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.HardwareVersion", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.SoftwareVersion", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.SerialNumber", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.VendorOUI", RBUS_ELEMENT_TYPE_PROPERTY, {manufacturer_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* OLT Info (3 parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.OLT.MACAddress", RBUS_ELEMENT_TYPE_PROPERTY, {olt_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.OLT.VendorOUI", RBUS_ELEMENT_TYPE_PROPERTY, {olt_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.OLT.VendorSpecificInfo", RBUS_ELEMENT_TYPE_PROPERTY, {olt_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Phase 7.1: LLID Dynamic Table (count only - instances registered dynamically) */
    {TR181_BASE_PATH ".X_RDK_EPON.LLIDNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Phase 7.2: DPoE/CPE Dynamic Table (4 stats + 1 count, table is dynamically registered) */
    {TR181_BASE_PATH ".X_RDK_EPON.DPoE.MaxCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPoE.StaticCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPoE.DynamicCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPENumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
};

/**
 * @brief Initialize TR-181 parameter handlers
 */
int eponMgr_tr181_init(rbusHandle_t handle) {
    if (!handle) {
        EPONMGR_LOG_ERROR("Invalid handle: %p\n", handle);
        return -1;
    }

    g_rbus_handle = handle;
    g_param_count = sizeof(g_tr181_params) / sizeof(rbusDataElement_t);

    EPONMGR_LOG_INFO("Registering %d TR-181 parameters with RBUS (including dynamic tables)\n", g_param_count);

    rbusError_t rc = rbus_regDataElements(handle, g_param_count, g_tr181_params);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to register TR-181 parameters: %d\n", rc);
        return -1;
    }

    EPONMGR_LOG_INFO("TR-181 parameter registration complete: %d parameters\n", g_param_count);
    
    /* Sync LLID table to register any existing LLIDs */
    eponMgr_tr181_sync_llid_table();
    
    /* Sync CPE table to register any existing CPEs */
    eponMgr_tr181_sync_cpe_table();
    
    return 0;
}

/**
 * @brief Register a single LLID instance dynamically
 * 
 * @param instance 1-based LLID instance number (1-32)
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_register_llid_instance(uint32_t instance) {
    if (!g_rbus_handle || instance == 0 || instance > MAX_LLID_INSTANCES) {
        EPONMGR_LOG_ERROR("Invalid LLID instance: %u\n", instance);
        return -1;
    }

    pthread_mutex_lock(&g_llid_table_mutex);

    /* Check if already registered */
    if (g_llid_instances[instance - 1].registered) {
        pthread_mutex_unlock(&g_llid_table_mutex);
        EPONMGR_LOG_DEBUG("LLID instance %u already registered\n", instance);
        return 0;
    }

    /* Build RBUS data elements for this LLID instance */
    char path_buf[256];
    rbusDataElement_t llid_params[6];

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.LLID\", instance);
    llid_params[0].name = strdup(path_buf);
    llid_params[0].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[0].cbTable.getHandler = llid_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.Status\", instance);
    llid_params[1].name = strdup(path_buf);
    llid_params[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[1].cbTable.getHandler = llid_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.MACAddress\", instance);
    llid_params[2].name = strdup(path_buf);
    llid_params[2].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[2].cbTable.getHandler = llid_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.Mode\", instance);
    llid_params[3].name = strdup(path_buf);
    llid_params[3].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[3].cbTable.getHandler = llid_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.EncryptionEnabled\", instance);
    llid_params[4].name = strdup(path_buf);
    llid_params[4].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[4].cbTable.getHandler = llid_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.ForwardingState\", instance);
    llid_params[5].name = strdup(path_buf);
    llid_params[5].type = RBUS_ELEMENT_TYPE_PROPERTY;
    llid_params[5].cbTable.getHandler = llid_table_handler;

    /* Register with RBUS */
    rbusError_t rc = rbus_regDataElements(g_rbus_handle, 6, llid_params);
    
    /* Free allocated strings */
    for (int i = 0; i < 6; i++) {
        free((void*)llid_params[i].name);
    }

    if (rc != RBUS_ERROR_SUCCESS) {
        pthread_mutex_unlock(&g_llid_table_mutex);
        EPONMGR_LOG_ERROR("Failed to register LLID instance %u: %d\n", instance, rc);
        return -1;
    }

    /* Mark as registered */
    g_llid_instances[instance - 1].instance = instance;
    g_llid_instances[instance - 1].registered = true;

    pthread_mutex_unlock(&g_llid_table_mutex);
    EPONMGR_LOG_INFO("Registered LLID instance %u\n", instance);
    return 0;
}

/**
 * @brief Unregister a single LLID instance dynamically
 * 
 * @param instance 1-based LLID instance number (1-32)
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_unregister_llid_instance(uint32_t instance) {
    if (!g_rbus_handle || instance == 0 || instance > MAX_LLID_INSTANCES) {
        EPONMGR_LOG_ERROR("Invalid LLID instance: %u\n", instance);
        return -1;
    }

    pthread_mutex_lock(&g_llid_table_mutex);

    /* Check if registered */
    if (!g_llid_instances[instance - 1].registered) {
        pthread_mutex_unlock(&g_llid_table_mutex);
        EPONMGR_LOG_DEBUG("LLID instance %u not registered\n", instance);
        return 0;
    }

    /* Build RBUS data elements for this LLID instance */
    char path_buf[256];
    rbusDataElement_t llid_params[6];

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.LLID\", instance);
    llid_params[0].name = strdup(path_buf);
    llid_params[0].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.Status\", instance);
    llid_params[1].name = strdup(path_buf);
    llid_params[1].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.MACAddress\", instance);
    llid_params[2].name = strdup(path_buf);
    llid_params[2].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.Mode\", instance);
    llid_params[3].name = strdup(path_buf);
    llid_params[3].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.EncryptionEnabled\", instance);
    llid_params[4].name = strdup(path_buf);
    llid_params[4].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH \".X_RDK_EPON.LLID.%u.ForwardingState\", instance);
    llid_params[5].name = strdup(path_buf);
    llid_params[5].type = RBUS_ELEMENT_TYPE_PROPERTY;

    /* Unregister from RBUS */
    rbusError_t rc = rbus_unregDataElements(g_rbus_handle, 6, llid_params);
    
    /* Free allocated strings */
    for (int i = 0; i < 6; i++) {
        free((void*)llid_params[i].name);
    }

    if (rc != RBUS_ERROR_SUCCESS) {
        pthread_mutex_unlock(&g_llid_table_mutex);
        EPONMGR_LOG_ERROR("Failed to unregister LLID instance %u: %d\n", instance, rc);
        return -1;
    }

    /* Mark as unregistered */
    g_llid_instances[instance - 1].registered = false;
    g_llid_instances[instance - 1].instance = 0;

    pthread_mutex_unlock(&g_llid_table_mutex);
    EPONMGR_LOG_INFO("Unregistered LLID instance %u\n", instance);
    return 0;
}

/**
 * @brief Synchronize LLID table registrations with current LLID list
 * 
 * Compares current registrations with actual LLID list and registers/unregisters as needed.
 * Should be called when LLID list changes.
 * 
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_sync_llid_table(void) {
    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper || !hal_wrapper->llid_list) {
        eponMgr_controller_unlock_hal_wrapper();
        EPONMGR_LOG_ERROR("HAL wrapper or LLID list not available\n");
        return -1;
    }

    uint32_t llid_count = eponMgr_llid_list_count(hal_wrapper->llid_list);
    
    /* Get current LLID instances from list */
    bool active_instances[MAX_LLID_INSTANCES] = {false};
    for (uint32_t i = 0; i < llid_count && i < MAX_LLID_INSTANCES; i++) {
        epon_llid_info_t llid_info;
        if (eponMgr_llid_list_get_at(hal_wrapper->llid_list, i, &llid_info) == 0) {
            /* Mark instance as active (1-based indexing) */
            active_instances[i] = true;
        }
    }

    eponMgr_controller_unlock_hal_wrapper();

    /* Register new instances and unregister removed ones */
    for (uint32_t i = 0; i < MAX_LLID_INSTANCES; i++) {
        uint32_t instance = i + 1;  /* 1-based */
        
        if (active_instances[i] && !g_llid_instances[i].registered) {
            /* New LLID - register it */
            eponMgr_tr181_register_llid_instance(instance);
        } else if (!active_instances[i] && g_llid_instances[i].registered) {
            /* Removed LLID - unregister it */
            eponMgr_tr181_unregister_llid_instance(instance);
        }
    }

    return 0;
}

/**
 * @brief Register a single CPE instance dynamically
 * 
 * @param instance 1-based CPE instance number (1-256)
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_register_cpe_instance(uint32_t instance) {
    if (!g_rbus_handle || instance == 0 || instance > MAX_CPE_INSTANCES) {
        EPONMGR_LOG_ERROR("Invalid CPE instance: %u\n", instance);
        return -1;
    }

    pthread_mutex_lock(&g_cpe_table_mutex);

    /* Check if already registered */
    if (g_cpe_instances[instance - 1].registered) {
        pthread_mutex_unlock(&g_cpe_table_mutex);
        EPONMGR_LOG_DEBUG("CPE instance %u already registered\n", instance);
        return 0;
    }

    /* Build RBUS data elements for this CPE instance */
    char path_buf[256];
    rbusDataElement_t cpe_params[3];

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.MACAddress", instance);
    cpe_params[0].name = strdup(path_buf);
    cpe_params[0].type = RBUS_ELEMENT_TYPE_PROPERTY;
    cpe_params[0].cbTable.getHandler = cpe_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.Type", instance);
    cpe_params[1].name = strdup(path_buf);
    cpe_params[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
    cpe_params[1].cbTable.getHandler = cpe_table_handler;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.AgeTime", instance);
    cpe_params[2].name = strdup(path_buf);
    cpe_params[2].type = RBUS_ELEMENT_TYPE_PROPERTY;
    cpe_params[2].cbTable.getHandler = cpe_table_handler;

    /* Register with RBUS */
    rbusError_t rc = rbus_regDataElements(g_rbus_handle, 3, cpe_params);
    
    /* Free allocated strings */
    for (int i = 0; i < 3; i++) {
        free((void*)cpe_params[i].name);
    }

    if (rc != RBUS_ERROR_SUCCESS) {
        pthread_mutex_unlock(&g_cpe_table_mutex);
        EPONMGR_LOG_ERROR("Failed to register CPE instance %u: %d\n", instance, rc);
        return -1;
    }

    /* Mark as registered */
    g_cpe_instances[instance - 1].instance = instance;
    g_cpe_instances[instance - 1].registered = true;

    pthread_mutex_unlock(&g_cpe_table_mutex);
    EPONMGR_LOG_INFO("Registered CPE instance %u\n", instance);
    return 0;
}

/**
 * @brief Unregister a single CPE instance dynamically
 * 
 * @param instance 1-based CPE instance number (1-256)
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_unregister_cpe_instance(uint32_t instance) {
    if (!g_rbus_handle || instance == 0 || instance > MAX_CPE_INSTANCES) {
        EPONMGR_LOG_ERROR("Invalid CPE instance: %u\n", instance);
        return -1;
    }

    pthread_mutex_lock(&g_cpe_table_mutex);

    /* Check if registered */
    if (!g_cpe_instances[instance - 1].registered) {
        pthread_mutex_unlock(&g_cpe_table_mutex);
        EPONMGR_LOG_DEBUG("CPE instance %u not registered\n", instance);
        return 0;
    }

    /* Build RBUS data elements for this CPE instance */
    char path_buf[256];
    rbusDataElement_t cpe_params[3];

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.MACAddress", instance);
    cpe_params[0].name = strdup(path_buf);
    cpe_params[0].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.Type", instance);
    cpe_params[1].name = strdup(path_buf);
    cpe_params[1].type = RBUS_ELEMENT_TYPE_PROPERTY;

    snprintf(path_buf, sizeof(path_buf), TR181_BASE_PATH ".X_RDK_EPON.DPoE.CPE.%u.AgeTime", instance);
    cpe_params[2].name = strdup(path_buf);
    cpe_params[2].type = RBUS_ELEMENT_TYPE_PROPERTY;

    /* Unregister from RBUS */
    rbusError_t rc = rbus_unregDataElements(g_rbus_handle, 3, cpe_params);
    
    /* Free allocated strings */
    for (int i = 0; i < 3; i++) {
        free((void*)cpe_params[i].name);
    }

    if (rc != RBUS_ERROR_SUCCESS) {
        pthread_mutex_unlock(&g_cpe_table_mutex);
        EPONMGR_LOG_ERROR("Failed to unregister CPE instance %u: %d\n", instance, rc);
        return -1;
    }

    /* Mark as unregistered */
    g_cpe_instances[instance - 1].registered = false;
    g_cpe_instances[instance - 1].instance = 0;

    pthread_mutex_unlock(&g_cpe_table_mutex);
    EPONMGR_LOG_INFO("Unregistered CPE instance %u\n", instance);
    return 0;
}

/**
 * @brief Synchronize CPE table registrations with current CPE list
 * 
 * Compares current registrations with actual CPE list and registers/unregisters as needed.
 * Should be called when CPE list changes.
 * 
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_sync_cpe_table(void) {
    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper || !hal_wrapper->cpe_list) {
        eponMgr_controller_unlock_hal_wrapper();
        EPONMGR_LOG_ERROR("HAL wrapper or CPE list not available\n");
        return -1;
    }

    uint32_t cpe_count = eponMgr_cpe_list_count(hal_wrapper->cpe_list);
    
    /* Get current CPE instances from list */
    bool active_instances[MAX_CPE_INSTANCES] = {false};
    for (uint32_t i = 0; i < cpe_count && i < MAX_CPE_INSTANCES; i++) {
        dpoe_cpe_mac_entry_t cpe_entry;
        if (eponMgr_cpe_list_get_at(hal_wrapper->cpe_list, i, &cpe_entry) == 0) {
            /* Mark instance as active (1-based indexing) */
            active_instances[i] = true;
        }
    }

    eponMgr_controller_unlock_hal_wrapper();

    /* Register new instances and unregister removed ones */
    for (uint32_t i = 0; i < MAX_CPE_INSTANCES; i++) {
        uint32_t instance = i + 1;  /* 1-based */
        
        if (active_instances[i] && !g_cpe_instances[i].registered) {
            /* New CPE - register it */
            eponMgr_tr181_register_cpe_instance(instance);
        } else if (!active_instances[i] && g_cpe_instances[i].registered) {
            /* Removed CPE - unregister it */
            eponMgr_tr181_unregister_cpe_instance(instance);
        }
    }

    return 0;
}

/**
 * @brief Cleanup TR-181 parameter handlers
 */
void eponMgr_tr181_cleanup(rbusHandle_t handle) {
    if (!handle || g_param_count == 0) {
        return;
    }

    /* Unregister all dynamic LLID instances */
    for (uint32_t i = 0; i < MAX_LLID_INSTANCES; i++) {
        if (g_llid_instances[i].registered) {
            eponMgr_tr181_unregister_llid_instance(i + 1);
        }
    }

    /* Unregister all dynamic CPE instances */
    for (uint32_t i = 0; i < MAX_CPE_INSTANCES; i++) {
        if (g_cpe_instances[i].registered) {
            eponMgr_tr181_unregister_cpe_instance(i + 1);
        }
    }

    EPONMGR_LOG_INFO("Unregistering %d TR-181 parameters from RBUS\n", g_param_count);
    rbus_unregDataElements(handle, g_param_count, g_tr181_params);

    g_rbus_handle = NULL;
    g_param_count = 0;

    EPONMGR_LOG_INFO("TR-181 parameter cleanup complete\n");
}

/**
 * @brief Get number of registered TR-181 parameters
 */
int eponMgr_tr181_get_param_count(void) {
    return g_param_count;
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Convert encryption mode enum to string
 */
static const char* encryption_mode_to_string(epon_encryption_mode_t mode) {
    switch (mode) {
        case EPON_ENCRYPTION_MODE_DISABLED: return "None";
        case EPON_ENCRYPTION_MODE_AES_128: return "AES";
        case EPON_ENCRYPTION_MODE_TRIPLE_CHURNING: return "TripleChurning";
        case EPON_ENCRYPTION_MODE_AES_256: return "AES256";
        default: return "Unknown";
    }
}

/**
 * @brief Convert ONU status enum to string
 */
static const char* onu_status_to_string(epon_onu_status_t status) {
    switch (status) {
        case EPON_ONU_STATUS_LOS: return "Unregistered";
        case EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED: return "Discovering";
        case EPON_ONU_STATUS_REGISTRATION: return "Registered";
        case EPON_ONU_STATUS_DEREGISTRATION: return "Deregistered";
        default: return "Unknown";
    }
}

/**
 * @brief Convert LLID state enum to string
 */
static const char* llid_state_to_string(epon_llid_state_t state) {
    switch (state) {
        case EPON_LLID_STATE_UNREGISTERED: return "Unregistered";
        case EPON_LLID_STATE_REGISTERING: return "Registering";
        case EPON_LLID_STATE_REGISTERED: return "Registered";
        case EPON_LLID_STATE_DEREGISTERING: return "Deregistering";
        case EPON_LLID_STATE_FAILED: return "Failed";
        default: return "Unknown";
    }
}

/* ============================================================================
 * GET/SET Handler Implementations
 * ============================================================================ */

/**
 * @brief Base parameter GET handler
 * 
 * Handles: Enable, Status, Alias, Name, LastChange, LowerLayers, Upstream
 */
static rbusError_t base_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    if (strstr(param_name, ".Enable")) {
        // Always return enabled for now - TODO: implement PSM-based config storage
        rbusValue_SetBoolean(value, true);
    }
    else if (strstr(param_name, ".Status")) {
        // Get status from ONU state
        epon_onu_status_t onu_status;
        if (hal_wrapper->onu_state) {
            pthread_mutex_lock(&hal_wrapper->onu_state->mutex);
            onu_status = hal_wrapper->onu_state->current_status;
            pthread_mutex_unlock(&hal_wrapper->onu_state->mutex);
            
            // Map ONU status to TR-181 status
            const char *status_str;
            if (onu_status == EPON_ONU_STATUS_REGISTRATION) {
                status_str = "Up";
            } else if (onu_status == EPON_ONU_STATUS_DEREGISTRATION) {
                status_str = "Down";
            } else if (onu_status == EPON_ONU_STATUS_LOS) {
                status_str = "NotPresent";
            } else {
                status_str = "Dormant";
            }
            rbusValue_SetString(value, status_str);
        } else {
            rbusValue_SetString(value, "Unknown");
        }
    }
    else if (strstr(param_name, ".Alias")) {
        // Return default alias - TODO: implement PSM-based config storage
        rbusValue_SetString(value, "OpticalInterface1");
    }
    else if (strstr(param_name, ".Name")) {
        rbusValue_SetString(value, "veip0");
    }
    else if (strstr(param_name, ".LastChange")) {
        if (hal_wrapper->onu_state) {
            pthread_mutex_lock(&hal_wrapper->onu_state->mutex);
            time_t now = time(NULL);
            uint32_t last_change = (uint32_t)(now - hal_wrapper->onu_state->last_status_change);
            pthread_mutex_unlock(&hal_wrapper->onu_state->mutex);
            rbusValue_SetUInt32(value, last_change);
        } else {
            rbusValue_SetUInt32(value, 0);
        }
    }
    else if (strstr(param_name, ".LowerLayers")) {
        rbusValue_SetString(value, "");  // No lower layers for EPON
    }
    else if (strstr(param_name, ".Upstream")) {
        rbusValue_SetBoolean(value, false);  // EPON is downstream from perspective of ONU
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Base parameter SET handler
 * 
 * Handles: Enable, Alias
 */
static rbusError_t base_param_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);

    EPONMGR_LOG_DEBUG("TR-181 SET: %s\n", param_name);

    if (strstr(param_name, ".Enable")) {
        bool enable = rbusValue_GetBoolean(value);
        
        // TODO: Store in PSM and trigger controller state machine event
        EPONMGR_LOG_INFO("Interface %s via TR-181 (not persisted yet)\n", enable ? "enabled" : "disabled");
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, ".Alias")) {
        const char* alias = rbusValue_GetString(value, NULL);
        
        // TODO: Store in PSM
        EPONMGR_LOG_INFO("Interface alias set to '%s' via TR-181 (not persisted yet)\n", alias);
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_INVALID_INPUT;
}

/**
 * @brief Optical parameter GET handler
 * 
 * Handles: OpticalSignalLevel, TransmitOpticalLevel, thresholds
 */
static rbusError_t optical_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get transceiver stats from HAL wrapper (with caching)
    epon_hal_transceiver_stats_t trans_stats;
    trans_stats.struct_size = sizeof(epon_hal_transceiver_stats_t);
    
    int ret = eponMgr_hal_wrapper_get_transceiver_stats(hal_wrapper, &trans_stats);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("Failed to get transceiver stats: %d\n", ret);
        rbusValue_Release(value);        eponMgr_controller_unlock_hal_wrapper();        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* All optical values are in 0.1 dBm units (int32) */
    if (strstr(param_name, "OpticalSignalLevel")) {
        int32_t rx_power = (int32_t)(trans_stats.optical_signal_level * 10.0f);
        rbusValue_SetInt32(value, rx_power);
    }
    else if (strstr(param_name, "TransmitOpticalLevel")) {
        int32_t tx_power = (int32_t)(trans_stats.transmit_optical_level * 10.0f);
        rbusValue_SetInt32(value, tx_power);
    }
    else if (strstr(param_name, "LowerOpticalThreshold")) {
        int32_t threshold = (int32_t)(trans_stats.lower_optical_threshold * 10.0f);
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "UpperOpticalThreshold")) {
        int32_t threshold = (int32_t)(trans_stats.upper_optical_threshold * 10.0f);
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "LowerTransmitPowerThreshold")) {
        int32_t threshold = (int32_t)(trans_stats.lower_transmit_power_threshold * 10.0f);
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "UpperTransmitPowerThreshold")) {
        int32_t threshold = (int32_t)(trans_stats.upper_transmit_power_threshold * 10.0f);
        rbusValue_SetInt32(value, threshold);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Statistics GET handler
 * 
 * Handles: All Stats.* parameters (standard + X_RDK)
 */
static rbusError_t stats_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get link stats from HAL wrapper (with 30s caching)
    epon_hal_link_stats_t link_stats;
    link_stats.struct_size = sizeof(epon_hal_link_stats_t);
    
    int ret = eponMgr_hal_wrapper_get_link_stats(hal_wrapper, &link_stats);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("Failed to get link stats: %d\n", ret);
        rbusValue_Release(value);        eponMgr_controller_unlock_hal_wrapper();        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* Standard Statistics */
    if (strstr(param_name, "BytesSent")) {
        rbusValue_SetUInt64(value, link_stats.bytes_sent);
    }
    else if (strstr(param_name, "BytesReceived")) {
        rbusValue_SetUInt64(value, link_stats.bytes_received);
    }
    else if (strstr(param_name, "PacketsSent")) {
        rbusValue_SetUInt64(value, link_stats.packets_sent);
    }
    else if (strstr(param_name, "PacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats.packets_received);
    }
    else if (strstr(param_name, "ErrorsSent")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.errors_sent);
    }
    else if (strstr(param_name, "ErrorsReceived")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.errors_received);
    }
    else if (strstr(param_name, "DiscardPacketsSent")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.discard_packets_sent);
    }
    else if (strstr(param_name, "DiscardPacketsReceived")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.discard_packets_received);
    }
    else if (strstr(param_name, "UnicastPacketsSent")) {
        // Unicast = Total - (Multicast + Broadcast)
        uint64_t unicast = link_stats.packets_sent - link_stats.multicast_packets_sent - link_stats.broadcast_packets_sent;
        rbusValue_SetUInt64(value, unicast);
    }
    else if (strstr(param_name, "UnicastPacketsReceived")) {
        uint64_t unicast = link_stats.packets_received - link_stats.multicast_packets_received - link_stats.broadcast_packets_received;
        rbusValue_SetUInt64(value, unicast);
    }
    else if (strstr(param_name, "MulticastPacketsSent")) {
        rbusValue_SetUInt64(value, link_stats.multicast_packets_sent);
    }
    else if (strstr(param_name, "MulticastPacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats.multicast_packets_received);
    }
    else if (strstr(param_name, "BroadcastPacketsSent")) {
        rbusValue_SetUInt64(value, link_stats.broadcast_packets_sent);
    }
    else if (strstr(param_name, "BroadcastPacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats.broadcast_packets_received);
    }
    else if (strstr(param_name, "UnknownProtoPacketsReceived")) {
        rbusValue_SetUInt32(value, 0);  // Not available in HAL
    }
    /* X_RDK Statistics */
    else if (strstr(param_name, "FECCorrected")) {
        rbusValue_SetUInt64(value, link_stats.fec_corrected);
    }
    else if (strstr(param_name, "FECUncorrectable")) {
        rbusValue_SetUInt64(value, link_stats.fec_uncorrectable);
    }
    else if (strstr(param_name, "BER")) {
        // Calculate BER from FEC stats (simple approximation)
        if (link_stats.bytes_received > 0) {
            uint64_t ber = (link_stats.fec_uncorrectable * 1000000000ULL) / link_stats.bytes_received;
            rbusValue_SetUInt64(value, ber);
        } else {
            rbusValue_SetUInt64(value, 0);
        }
    }
    else if (strstr(param_name, "RangingResyncs")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.ranging_resyncs);
    }
    else if (strstr(param_name, "MACResets")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats.mac_resets);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Transceiver GET handler
 * 
 * Handles: Temperature, SupplyVoltage, BiasCurrent
 */
static rbusError_t transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get transceiver stats from HAL wrapper
    epon_hal_transceiver_stats_t trans_stats;
    trans_stats.struct_size = sizeof(epon_hal_transceiver_stats_t);
    
    int ret = eponMgr_hal_wrapper_get_transceiver_stats(hal_wrapper, &trans_stats);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("Failed to get transceiver stats: %d\n", ret);
        rbusValue_Release(value);        eponMgr_controller_unlock_hal_wrapper();        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, "Temperature")) {
        int32_t temp = (int32_t)(trans_stats.temperature * 10.0f);  /* 0.1°C units */
        rbusValue_SetInt32(value, temp);
    }
    else if (strstr(param_name, "SupplyVoltage")) {
        int32_t voltage = (int32_t)(trans_stats.supply_voltage * 1000.0f);  /* mV units */
        rbusValue_SetInt32(value, voltage);
    }
    else if (strstr(param_name, "BiasCurrent")) {
        int32_t current = (int32_t)(trans_stats.bias_current * 10.0f);  /* 0.1 mA units */
        rbusValue_SetInt32(value, current);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief EPON-specific parameter GET handler
 * 
 * Handles: OperationalMode, EncryptionMode, ONUStatus, DPoESupported, MaxLLIDSupported
 */
static rbusError_t epon_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    if (strstr(param_name, "OperationalMode")) {
        // Get link info from HAL wrapper
        epon_hal_link_info_t link_info;
        link_info.mode[0] = '\0';
        
        if (eponMgr_hal_wrapper_get_link_info(hal_wrapper, &link_info) == 0) {
            rbusValue_SetString(value, link_info.mode);
        } else {
            rbusValue_SetString(value, "1G-EPON");  // Default
        }
    }
    else if (strstr(param_name, "EncryptionMode")) {
        epon_hal_link_info_t link_info;
        
        if (eponMgr_hal_wrapper_get_link_info(hal_wrapper, &link_info) == 0) {
            const char *enc_str = encryption_mode_to_string(link_info.encryption);
            rbusValue_SetString(value, enc_str);
        } else {
            rbusValue_SetString(value, "None");
        }
    }
    else if (strstr(param_name, "ONUStatus")) {
        if (hal_wrapper->onu_state) {
            pthread_mutex_lock(&hal_wrapper->onu_state->mutex);
            const char *status_str = onu_status_to_string(hal_wrapper->onu_state->current_status);
            pthread_mutex_unlock(&hal_wrapper->onu_state->mutex);
            rbusValue_SetString(value, status_str);
        } else {
            rbusValue_SetString(value, "Unregistered");
        }
    }
    else if (strstr(param_name, "DPoESupported")) {
        rbusValue_SetBoolean(value, hal_wrapper->hal_config.dpoe_supported);
    }
    else if (strstr(param_name, "MaxLLIDSupported")) {
        if (hal_wrapper->llid_list) {
            pthread_mutex_lock(&hal_wrapper->llid_list->mutex);
            uint32_t max_llid = hal_wrapper->llid_list->llid_list.max_llid_count;
            pthread_mutex_unlock(&hal_wrapper->llid_list->mutex);
            rbusValue_SetUInt32(value, max_llid);
        } else {
            rbusValue_SetUInt32(value, 1);  // Default single LLID
        }
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Manufacturer info GET handler
 * 
 * Handles: Manufacturer, ModelNumber, HardwareVersion, SoftwareVersion, SerialNumber, VendorOUI
 */
static rbusError_t manufacturer_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get manufacturer info from HAL wrapper
    epon_onu_manufacturer_info_t mfr_info;
    mfr_info.struct_size = sizeof(epon_onu_manufacturer_info_t);
    
    int ret = eponMgr_hal_wrapper_get_onu_manufacturer_info(hal_wrapper, &mfr_info);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("Failed to get manufacturer info: %d\n", ret);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, ".Manufacturer")) {
        rbusValue_SetString(value, mfr_info.manufacturer);
    }
    else if (strstr(param_name, "ModelNumber")) {
        rbusValue_SetString(value, mfr_info.model_number);
    }
    else if (strstr(param_name, "HardwareVersion")) {
        rbusValue_SetString(value, mfr_info.hardware_version);
    }
    else if (strstr(param_name, "SoftwareVersion")) {
        rbusValue_SetString(value, mfr_info.software_version);
    }
    else if (strstr(param_name, "SerialNumber")) {
        rbusValue_SetString(value, mfr_info.serial_number);
    }
    else if (strstr(param_name, "VendorOUI")) {
        char oui_str[16];
        snprintf(oui_str, sizeof(oui_str), "%02X%02X%02X", 
                 mfr_info.vendor_oui[0], mfr_info.vendor_oui[1], mfr_info.vendor_oui[2]);
        rbusValue_SetString(value, oui_str);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief OLT information GET handler
 * 
 * Handles: OLT.MACAddress, OLT.VendorOUI, OLT.VendorSpecificInfo
 */
static rbusError_t olt_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get OLT info from HAL wrapper
    epon_olt_info_t olt_info;
    olt_info.struct_size = sizeof(epon_olt_info_t);
    
    int ret = eponMgr_hal_wrapper_get_olt_info(hal_wrapper, &olt_info);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("Failed to get OLT info: %d\n", ret);
        rbusValue_Release(value);        eponMgr_controller_unlock_hal_wrapper();        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, "MACAddress")) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 olt_info.mac_address[0], olt_info.mac_address[1], olt_info.mac_address[2],
                 olt_info.mac_address[3], olt_info.mac_address[4], olt_info.mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strstr(param_name, "VendorOUI")) {
        char oui_str[16];
        snprintf(oui_str, sizeof(oui_str), "%02X%02X%02X",
                 olt_info.vendor_oui[0], olt_info.vendor_oui[1], olt_info.vendor_oui[2]);
        rbusValue_SetString(value, oui_str);
    }
    else if (strstr(param_name, "VendorSpecificInfo")) {
        rbusValue_SetString(value, "");  // Not available in current HAL
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/* ============================================================================
 * Phase 7.1: LLID Dynamic Table Handler
 * ============================================================================ */

/**
 * @brief LLID table handler
 * 
 * Handles: LLIDNumberOfEntries, LLID.{i}.* parameters
 */
static rbusError_t llid_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper || !hal_wrapper->llid_list) {
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Handle count parameter
    if (strstr(param_name, "LLIDNumberOfEntries")) {
        uint32_t count = eponMgr_llid_list_count(hal_wrapper->llid_list);
        rbusValue_SetUInt32(value, count);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_SUCCESS;
    }

    // Handle table instance parameters (LLID.{i}.*)
    // Extract instance number from parameter name
    const char *llid_prefix = ".X_RDK_EPON.LLID.";
    const char *instance_start = strstr(param_name, llid_prefix);
    if (!instance_start) {
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_INVALID_INPUT;
    }

    instance_start += strlen(llid_prefix);
    uint32_t instance = 0;
    sscanf(instance_start, "%u", &instance);

    if (instance == 0 || instance > 32) {
        EPONMGR_LOG_ERROR("Invalid LLID instance: %u\n", instance);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Get LLID info from list (instance is 1-based)
    epon_llid_info_t llid_info;
    int ret = eponMgr_llid_list_get_at(hal_wrapper->llid_list, instance - 1, &llid_info);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("LLID instance %u not found\n", instance);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    // Determine which LLID parameter is requested
    if (strstr(param_name, ".LLID")) {
        rbusValue_SetUInt32(value, llid_info.llid_value);
    }
    else if (strstr(param_name, ".Status")) {
        const char *status_str = llid_state_to_string(llid_info.state);
        rbusValue_SetString(value, status_str);
    }
    else if (strstr(param_name, ".MACAddress")) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 llid_info.local_mac_address[0], llid_info.local_mac_address[1],
                 llid_info.local_mac_address[2], llid_info.local_mac_address[3],
                 llid_info.local_mac_address[4], llid_info.local_mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strstr(param_name, ".Mode")) {
        const char *mode_str = (llid_info.mode == EPON_LLID_MODE_UNICAST) ? "Unicast" : "Broadcast";
        rbusValue_SetString(value, mode_str);
    }
    else if (strstr(param_name, ".EncryptionEnabled")) {
        rbusValue_SetBoolean(value, llid_info.encryption_enabled);
    }
    else if (strstr(param_name, ".ForwardingState")) {
        const char *fwd_str;
        if (llid_info.forwarding_state == EPON_LLID_FORWARDING_ENABLED) {
            fwd_str = "Enabled";
        } else if (llid_info.forwarding_state == EPON_LLID_FORWARDING_LEARNING) {
            fwd_str = "Learning";
        } else {
            fwd_str = "Disabled";
        }
        rbusValue_SetString(value, fwd_str);
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}

/* ============================================================================
 * Phase 7.2: DPoE/CPE Dynamic Table Handler
 * ============================================================================ */

/**
 * @brief CPE table handler
 * 
 * Handles: DPoE stats, CPENumberOfEntries, CPE.{i}.* parameters
 */
static rbusError_t cpe_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)eponMgr_controller_lock_hal_wrapper();
    if (!hal_wrapper || !hal_wrapper->cpe_list) {
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Handle DPoE statistics
    if (strstr(param_name, "MaxCPECount")) {
        pthread_mutex_lock(&hal_wrapper->cpe_list->mutex);
        uint32_t max_cpe = hal_wrapper->cpe_list->cpe_table.max_cpe;
        pthread_mutex_unlock(&hal_wrapper->cpe_list->mutex);
        rbusValue_SetUInt32(value, max_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "StaticCPECount")) {
        pthread_mutex_lock(&hal_wrapper->cpe_list->mutex);
        uint32_t static_cpe = hal_wrapper->cpe_list->cpe_table.static_cpe_count;
        pthread_mutex_unlock(&hal_wrapper->cpe_list->mutex);
        rbusValue_SetUInt32(value, static_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "DynamicCPECount")) {
        pthread_mutex_lock(&hal_wrapper->cpe_list->mutex);
        uint32_t dynamic_cpe = hal_wrapper->cpe_list->cpe_table.dynamic_cpe_count;
        pthread_mutex_unlock(&hal_wrapper->cpe_list->mutex);
        rbusValue_SetUInt32(value, dynamic_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "CPENumberOfEntries")) {
        uint32_t count = eponMgr_cpe_list_count(hal_wrapper->cpe_list);
        rbusValue_SetUInt32(value, count);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_SUCCESS;
    }

    // Handle table instance parameters (CPE.{i}.*)
    const char *cpe_prefix = ".DPoE.CPE.";
    const char *instance_start = strstr(param_name, cpe_prefix);
    if (!instance_start) {
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_INVALID_INPUT;
    }

    instance_start += strlen(cpe_prefix);
    uint32_t instance = 0;
    sscanf(instance_start, "%u", &instance);

    if (instance == 0 || instance > 256) {
        EPONMGR_LOG_ERROR("Invalid CPE instance: %u\n", instance);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Get CPE entry from list (instance is 1-based)
    dpoe_cpe_mac_entry_t cpe_entry;
    int ret = eponMgr_cpe_list_get_at(hal_wrapper->cpe_list, instance - 1, &cpe_entry);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("CPE instance %u not found\n", instance);
        rbusValue_Release(value);
        eponMgr_controller_unlock_hal_wrapper();
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    // Determine which CPE parameter is requested
    if (strstr(param_name, ".MACAddress")) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 cpe_entry.mac_address[0], cpe_entry.mac_address[1],
                 cpe_entry.mac_address[2], cpe_entry.mac_address[3],
                 cpe_entry.mac_address[4], cpe_entry.mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strstr(param_name, ".AddedTime")) {
        char time_str[32];
        time_t added_time = time(NULL) - cpe_entry.age_time;
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", localtime(&added_time));
        rbusValue_SetString(value, time_str);
    }
    else if (strstr(param_name, ".Type")) {
        const char *type_str = (cpe_entry.type == DPOE_CPE_MAC_STATIC) ? "Static" : "Dynamic";
        rbusValue_SetString(value, type_str);
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_controller_unlock_hal_wrapper();
    return RBUS_ERROR_SUCCESS;
}
