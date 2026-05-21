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
 * @file eponMgr_tr181.c
 * @brief TR-181 Parameter Handler Implementation with Real HAL Integration
 * 
 * Implements GET/SET handlers for Device.Optical.Interface.{i} parameters.
 * Maps TR-181 parameters to EPON HAL wrapper APIs with caching.
 * 
 * Includes dynamic table support for:
 * - LLID.{i} table (Phase 7.1)
 * - DPOE.CPE.{i} table (Phase 7.2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "eponMgr_tr181.h"
#include "eponMgr_logger.h"
#include "eponMgr_controller.h"
#include "eponMgr_persistence.h"
#include "eponMgr_psm.h"
#include "eponMgr_stats_poller.h"
#include "eponMgr_statsData.h"

#include <rbus/rbus.h>

/* TR-181 Base Path */
#define TR181_BASE_PATH "Device.Optical.Interface.1"
#define MAX_LLID_INSTANCES 64
#define MAX_CPE_INSTANCES 256
#define MAX_VEIP_INSTANCES 16

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

/* Dynamic VEIP Interface table tracking */
typedef struct {
    uint32_t instance;  /* 1-based instance number */
    char name[64];      /* Interface name (e.g., veip0) */
    bool registered;
} veip_instance_t;

/* Static state */
static rbusHandle_t g_rbus_handle = NULL;
static int g_param_count = 0;
static llid_instance_t g_llid_instances[MAX_LLID_INSTANCES] = {{0}};
static cpe_instance_t g_cpe_instances[MAX_CPE_INSTANCES] = {{0}};
static veip_instance_t g_veip_instances[MAX_VEIP_INSTANCES] = {{0}};

/**
 * @brief Print all registered TR-181 parameters
 * 
 * Iterates through g_tr181_params and prints element name and type.
 */
static void print_tr181_params(void);

/* Forward declarations for handlers */
static rbusError_t base_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t base_param_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);
static rbusError_t optical_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t stats_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t stats_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);
static rbusError_t transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t epon_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t manufacturer_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t olt_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t llid_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t cpe_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t veip_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t stats_poller_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
static rbusError_t stats_poller_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);
static rbusError_t epon_reset_method_handler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);
static rbusError_t epon_factory_reset_method_handler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

/**
 * @brief TR-181 Parameter Registration Table
 * 
 * Each entry defines a TR-181 parameter with its RBUS handlers.
 * Parameters are organized by category for easier maintenance.
 */
static rbusDataElement_t g_tr181_params[] = {
    /* Base Interface Parameters (8 parameters) */
    {TR181_BASE_PATH ".Enable", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, base_param_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Status", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Alias", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, base_param_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Name", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".LowerLayers", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Upstream", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".MaxBitRate", RBUS_ELEMENT_TYPE_PROPERTY, {base_param_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Optical Parameters (3 parameters) - Thresholds moved to X_RDK_Transceiver per BBF TR-181 v2.18 */
    {TR181_BASE_PATH ".OpticalSignalLevel", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".TransmitOpticalLevel", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".SFPReferenceList", RBUS_ELEMENT_TYPE_PROPERTY, {optical_param_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Standard Stats (9 parameters) - Per BBF TR-181 Device.Optical.Interface.Stats */
    {TR181_BASE_PATH ".Stats.Reset", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, stats_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* X_RDK Stats (11 parameters) */
    {TR181_BASE_PATH ".Stats.X_RDK_FECCorrected", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_FECUncorrectable", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_BER", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_RangingResyncs", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".Stats.X_RDK_MACResets", RBUS_ELEMENT_TYPE_PROPERTY, {stats_get_handler, NULL, NULL, NULL, NULL, NULL}},

    /* X_RDK_Transceiver (7 parameters) - Includes optical thresholds moved from BBF-deleted standard path */
    {TR181_BASE_PATH ".X_RDK_Transceiver.Temperature", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.SupplyVoltage", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.BiasCurrent", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.LowerOpticalThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.UpperOpticalThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.LowerTransmitPowerThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_Transceiver.UpperTransmitPowerThreshold", RBUS_ELEMENT_TYPE_PROPERTY, {transceiver_get_handler, NULL, NULL, NULL, NULL, NULL}},

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

    /* Phase 7.1: LLID Dynamic Table (table + count + row parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.LLIDNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.LLID", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.MACAddress", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.Mode", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.EncryptionEnabled", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.LLID.{i}.ForwardingState", RBUS_ELEMENT_TYPE_PROPERTY, {llid_table_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Phase 7.2: DPOE/CPE Dynamic Table (table + stats + row parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.MaxCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.StaticCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.DynamicCPECount", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPENumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.{i}.MACAddress", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.{i}.Type", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.{i}.AgeTime", RBUS_ELEMENT_TYPE_PROPERTY, {cpe_table_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Phase 7.3: VEIP Interface Dynamic Table (table + count + row parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.VEIP_InterfaceNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {veip_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {veip_table_handler, NULL, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {veip_table_handler, NULL, NULL, NULL, NULL, NULL}},

    /* Stats Poller Configuration (2 parameters) */
    {TR181_BASE_PATH ".X_RDK_EPON.StatsPoller.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {stats_poller_get_handler, stats_poller_set_handler, NULL, NULL, NULL, NULL}},
    {TR181_BASE_PATH ".X_RDK_EPON.StatsPoller.PollingInterval", RBUS_ELEMENT_TYPE_PROPERTY, {stats_poller_get_handler, stats_poller_set_handler, NULL, NULL, NULL, NULL}},

    /* RBUS Methods (2 methods) */
    {TR181_BASE_PATH ".X_RDK_EPON.Reset()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, (void*)epon_reset_method_handler}},
    {TR181_BASE_PATH ".X_RDK_EPON.FactoryReset()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, (void*)epon_factory_reset_method_handler}},
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
    /* Print all registered TR-181 parameters */
    print_tr181_params();

    /* Sync LLID table to register any existing LLIDs */
    eponMgr_tr181_sync_llid_table();
    
    /* Sync CPE table to register any existing CPEs */
    eponMgr_tr181_sync_cpe_table();
    
    /* Sync VEIP Interface table to register any existing interfaces */
    eponMgr_tr181_sync_veip_table();
    
    return 0;
}

/**
 * @brief Synchronize LLID table registrations with current LLID list
 * 
 * Compares current RBUS registrations with actual LLID list state
 * and registers/unregisters instances as needed. Should be called when
 * LLID list changes (add/remove events).
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Acquires data lock internally, safe to call from any thread
 * @note Automatically registers new LLID instances in TR-181 table
 * @note Uses 1-based indexing for TR-181 instance numbers
 */
int eponMgr_tr181_sync_llid_table(void) {
    if (!g_rbus_handle) {
        return -1;
    }

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return -1;
    }

    uint32_t llid_count = eponMgr_data_get_llid_count(eponData);
    /* Register LLID instances (1-based indexing) */
    for (uint32_t i = 0; i < llid_count && i < MAX_LLID_INSTANCES; i++) {
        uint32_t instance = i + 1;
        if (!g_llid_instances[instance - 1].registered) {
            epon_llid_info_t llid_info;
            if (eponMgr_data_get_llid_at_index(eponData, i, &llid_info) == 0) {
                
                /* Register this LLID instance */
                rbusError_t rc = rbusTable_registerRow(g_rbus_handle, TR181_BASE_PATH ".X_RDK_EPON.LLID.", instance, NULL);
                if (rc == RBUS_ERROR_SUCCESS) {
                    g_llid_instances[instance - 1].instance = instance;
                    g_llid_instances[instance - 1].registered = true;
                    EPONMGR_LOG_INFO("Registered LLID table row %s%u on-demand\n", TR181_BASE_PATH ".X_RDK_EPON.LLID.", instance);
                } else {
                    EPONMGR_LOG_ERROR("Failed to register LLID table row %u: %d\n", instance, rc);
                }
            }
        }
    }   

    eponMgr_data_unlock();
    return 0;
}

/**
 * @brief Synchronize CPE table registrations with current CPE list
 * 
 * Compares current RBUS registrations with actual CPE list state
 * and registers/unregisters instances as needed. Should be called when
 * CPE list changes (add/remove events) in DPoE mode.
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Acquires data lock internally, safe to call from any thread
 * @note Only relevant when DPoE is enabled
 * @note Automatically registers new CPE instances in TR-181 table
 */
int eponMgr_tr181_sync_cpe_table(void) {
    if (!g_rbus_handle) {
        return -1;
    }

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return -1;
    }

    uint32_t cpe_count = eponMgr_data_get_cpe_count(eponData);
    
    /* Register CPE instances (1-based indexing) */
    for (uint32_t i = 0; i < cpe_count && i < MAX_CPE_INSTANCES; i++) {
        uint32_t instance = i + 1;
        if (!g_cpe_instances[instance - 1].registered) {
            dpoe_cpe_mac_entry_t cpe_entry;
            if (eponMgr_data_get_cpe_at_index(eponData, i, &cpe_entry) == 0) {
                /* Register this CPE instance */
                rbusError_t rc = rbusTable_registerRow(g_rbus_handle, TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.", instance, NULL);
                if (rc == RBUS_ERROR_SUCCESS) {
                    g_cpe_instances[instance - 1].instance = instance;
                    g_cpe_instances[instance - 1].registered = true;
                    EPONMGR_LOG_INFO("Registered CPE table row %s%u on-demand\n", TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.", instance);
                }
            }
        }
    }

    eponMgr_data_unlock();
    return 0;
}

/**
 * @brief Cleanup TR-181 parameter handlers and unregister from RBUS
 * 
 * Unregisters all dynamic table instances (VEIP, LLID, CPE) and base
 * parameters from RBUS. This should be called during shutdown before
 * closing the RBUS connection.
 * 
 * @param handle RBUS handle
 * 
 * @note Safe to call with NULL handle or when already cleaned up
 * @note Unregisters all table rows in reverse order (VEIP, LLID, CPE)
 * @note Clears internal registration state for all instances
 */
void eponMgr_tr181_cleanup(rbusHandle_t handle) {
    if (!handle || g_param_count == 0) {
        return;
    }

    /* Unregister all dynamic VEIP instances */
    for (uint32_t i = 0; i < MAX_VEIP_INSTANCES; i++) {
        if (g_veip_instances[i].registered) {
            char row_name[256];
            snprintf(row_name, sizeof(row_name), TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.%u.", i + 1);
            rbusTable_unregisterRow(handle, row_name);
            g_veip_instances[i].registered = false;
        }
    }

    /* Unregister all dynamic LLID instances */
    for (uint32_t i = 0; i < MAX_LLID_INSTANCES; i++) {
        if (g_llid_instances[i].registered) {
            char row_name[256];
            snprintf(row_name, sizeof(row_name), TR181_BASE_PATH ".X_RDK_EPON.LLID.");
            rbusTable_unregisterRow(handle, row_name);
            g_llid_instances[i].registered = false;
        }
    }

    /* Unregister all dynamic CPE instances */
    for (uint32_t i = 0; i < MAX_CPE_INSTANCES; i++) {
        if (g_cpe_instances[i].registered) {
            char row_name[256];
            snprintf(row_name, sizeof(row_name), TR181_BASE_PATH ".X_RDK_EPON.DPOE.CPE.");
            rbusTable_unregisterRow(handle, row_name);
            g_cpe_instances[i].registered = false;
        }
    }

    EPONMGR_LOG_INFO("Unregistering %d TR-181 parameters from RBUS\n", g_param_count);
    rbus_unregDataElements(handle, g_param_count, g_tr181_params);

    g_rbus_handle = NULL;
    g_param_count = 0;

    EPONMGR_LOG_INFO("TR-181 parameter cleanup complete\n");
}

/**
 * @brief Get number of TR-181 parameters registered
 * 
 * Returns the total count of registered TR-181 parameters including
 * base parameters and dynamically registered table instances.
 * 
 * @return Number of registered parameters (base + dynamic tables)
 * 
 * @note Returns 0 if TR-181 has not been initialized
 * @note Count increases as dynamic table rows are registered
 */
int eponMgr_tr181_get_param_count(void) {
    return g_param_count;
}

/**
 * @brief Get capability string for a parameter based on its handlers
 * 
 * @param elem Data element to check
 * @return String describing parameter capabilities (ReadOnly, Read+Write, etc.)
 */
static const char* get_param_capability(const rbusDataElement_t *elem) {
    static char capability[64];
    capability[0] = '\0';
    
    int has_get = (elem->cbTable.getHandler != NULL);
    int has_set = (elem->cbTable.setHandler != NULL);
    int has_event = (elem->cbTable.eventSubHandler != NULL);
    int has_add_row = (elem->cbTable.tableAddRowHandler != NULL);
    int has_remove_row = (elem->cbTable.tableRemoveRowHandler != NULL);
    
    if (elem->type == RBUS_ELEMENT_TYPE_PROPERTY) {
        if (has_get && has_set) {
            strcat(capability, "Read +Write");
        } else if (has_get) {
            strcat(capability, "ReadOnly");
        }
        if (has_event) {
            strcat(capability, " +Sub");
        }
    }
    else if (elem->type == RBUS_ELEMENT_TYPE_TABLE) {
        strcat(capability, "Table");
        if (has_add_row) strcat(capability, " +Add");
        if (has_remove_row) strcat(capability, " +Del");
    }
    else if (elem->type == RBUS_ELEMENT_TYPE_EVENT) {
        strcat(capability, "Event");
    }
    else if (elem->type == RBUS_ELEMENT_TYPE_METHOD) {
        strcat(capability, "Method");
    }
    
    return (capability[0] != '\0') ? capability : "None";
}

/**
 * @brief Print all registered TR-181 parameters
 * 
 * Logs parameter information in a formatted table for debugging.
 */
void print_tr181_params(void) {
    EPONMGR_LOG_INFO("=== Registered TR-181 Parameters ===\n");
    EPONMGR_LOG_INFO("Total count: %d\n", g_param_count);
    EPONMGR_LOG_INFO("%-5s | %-10s | %-15s | %s\n", "Index", "Type", "Capability", "Parameter Name");
    EPONMGR_LOG_INFO("------|------------|-----------------|------------------------------------------------------------\n");
    
    for (int i = 0; i < g_param_count; i++) {
        const char *type_str;
        switch (g_tr181_params[i].type) {
            case RBUS_ELEMENT_TYPE_PROPERTY:
                type_str = "PROPERTY";
                break;
            case RBUS_ELEMENT_TYPE_TABLE:
                type_str = "TABLE";
                break;
            case RBUS_ELEMENT_TYPE_EVENT:
                type_str = "EVENT";
                break;
            case RBUS_ELEMENT_TYPE_METHOD:
                type_str = "METHOD";
                break;
            default:
                type_str = "UNKNOWN";
                break;
        }
        const char *capability = get_param_capability(&g_tr181_params[i]);
        EPONMGR_LOG_INFO("%-5d | %-10s | %-15s | %s\n", i + 1, type_str, capability, g_tr181_params[i].name);
    }
    EPONMGR_LOG_INFO("--------------------------------------------------------------------------------------------------\n\n");
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Convert encryption mode enum to string
 * 
 * @param mode Encryption mode enum
 * @return String representation of encryption mode
 */
static const char* encryption_mode_to_string(epon_encryption_mode_t mode) {
    switch (mode) {
        case EPON_ENCRYPTION_MODE_DISABLED: return "Disabled";
        case EPON_ENCRYPTION_MODE_AES_128: return "AES-128";
        case EPON_ENCRYPTION_MODE_TRIPLE_CHURNING: return "TripleChurning";
        case EPON_ENCRYPTION_MODE_AES_256: return "AES-256";
        default: return "Unknown";
    }
}

/**
 * @brief Convert ONU status enum to string
 * 
 * @param status ONU status enum
 * @return String representation of ONU status
 */
static const char* onu_status_to_string(epon_onu_status_t status) {
    switch (status) {
        case EPON_ONU_STATUS_LOS: return "LOS";
        case EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED: return "DownstreamSignalDetected";
        case EPON_ONU_STATUS_REGISTRATION: return "Registration";
        case EPON_ONU_STATUS_DEREGISTRATION: return "Deregistration";
        default: return "Unknown";
    }
}

/**
 * @brief Convert LLID state enum to string
 * 
 * @param state LLID state enum
 * @return String representation of LLID state
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    if (strstr(param_name, ".Enable")) {
        // Always return enabled for now 
        rbusValue_SetBoolean(value, true);
    }
    else if (strstr(param_name, ".Status")) {
        // Get status from ONU state
        epon_onu_status_t onu_status;
        if (eponData->onu_state) {
            pthread_mutex_lock(&eponData->onu_state->mutex);
            onu_status = eponData->onu_state->current_status;
            pthread_mutex_unlock(&eponData->onu_state->mutex);
            
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
        if (eponData->onu_state) {
            pthread_mutex_lock(&eponData->onu_state->mutex);
            time_t now = time(NULL);
            uint32_t last_change = (uint32_t)(now - eponData->onu_state->last_status_change);
            pthread_mutex_unlock(&eponData->onu_state->mutex);
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
    else if (strstr(param_name, ".MaxBitRate")) {
        // MaxBitRate at interface level per BBF TR-181 v2.18 (-1 = auto)
        const epon_hal_link_stats_t* link_stats = eponMgr_data_get_link_stats(eponData);
        if (link_stats) {
            rbusValue_SetInt32(value, (int32_t)link_stats->max_bit_rate);
        } else {
            rbusValue_SetInt32(value, -1);  // Default: auto
        }
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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
 * Handles: OpticalSignalLevel, TransmitOpticalLevel, SFPReferenceList
 */
static rbusError_t optical_param_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get transceiver stats from HAL wrapper (with caching)
    const epon_hal_transceiver_stats_t* trans_stats = eponMgr_data_get_transceiver_stats(eponData);
    if (!trans_stats) {
        EPONMGR_LOG_ERROR("Failed to get transceiver stats\n");
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* All optical values are in Dbm1000 units (dBm * 1000, int32) */
    if (strstr(param_name, "OpticalSignalLevel")) {
        int32_t rx_power = (int32_t)(trans_stats->optical_signal_level * 1000.0f);
        rbusValue_SetInt32(value, rx_power);
    }
    else if (strstr(param_name, "TransmitOpticalLevel")) {
        int32_t tx_power = (int32_t)(trans_stats->transmit_optical_level * 1000.0f);
        rbusValue_SetInt32(value, tx_power);
    }
    else if (strstr(param_name, "SFPReferenceList")) {
        rbusValue_SetString(value, "");
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get link stats from HAL wrapper (with caching)
    const epon_hal_link_stats_t* link_stats = eponMgr_data_get_link_stats(eponData);
    
    if (!link_stats) {
        EPONMGR_LOG_ERROR("Failed to get link stats\n");
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* Standard Statistics */
    if (strstr(param_name, ".Reset")) {
        // Stats.Reset is a command param - GET always returns false
        rbusValue_SetBoolean(value, false);
    }
    else if (strstr(param_name, "BytesSent")) {
        rbusValue_SetUInt64(value, link_stats->bytes_sent);
    }
    else if (strstr(param_name, "BytesReceived")) {
        rbusValue_SetUInt64(value, link_stats->bytes_received);
    }
    else if (strstr(param_name, "PacketsSent")) {
        rbusValue_SetUInt64(value, link_stats->packets_sent);
    }
    else if (strstr(param_name, "PacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats->packets_received);
    }
    else if (strstr(param_name, "ErrorsSent")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->errors_sent);
    }
    else if (strstr(param_name, "ErrorsReceived")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->errors_received);
    }
    else if (strstr(param_name, "DiscardPacketsSent")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->discard_packets_sent);
    }
    else if (strstr(param_name, "DiscardPacketsReceived")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->discard_packets_received);
    }
    /* X_RDK Statistics */
    else if (strstr(param_name, "FECCorrected")) {
        rbusValue_SetUInt64(value, link_stats->fec_corrected);
    }
    else if (strstr(param_name, "FECUncorrectable")) {
        rbusValue_SetUInt64(value, link_stats->fec_uncorrectable);
    }
    else if (strstr(param_name, "X_RDK_BroadcastPacketsSent")) {
        rbusValue_SetUInt64(value, link_stats->broadcast_packets_sent);
    }
    else if (strstr(param_name, "X_RDK_BroadcastPacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats->broadcast_packets_received);
    }
    else if (strstr(param_name, "X_RDK_MulticastPacketsSent")) {
        rbusValue_SetUInt64(value, link_stats->multicast_packets_sent);
    }
    else if (strstr(param_name, "X_RDK_MulticastPacketsReceived")) {
        rbusValue_SetUInt64(value, link_stats->multicast_packets_received);
    }
    else if (strstr(param_name, "X_RDK_UnicastPacketsSent")) {
        // Unicast = Total - (Multicast + Broadcast)
        uint64_t unicast = link_stats->packets_sent - link_stats->multicast_packets_sent - link_stats->broadcast_packets_sent;
        rbusValue_SetUInt64(value, unicast);
    }
    else if (strstr(param_name, "X_RDK_UnicastPacketsReceived")) {
        uint64_t unicast = link_stats->packets_received - link_stats->multicast_packets_received - link_stats->broadcast_packets_received;
        rbusValue_SetUInt64(value, unicast);
    }
    else if (strstr(param_name, "BER")) {
        // Calculate BER from FEC corrected bit errors (returns string in scientific notation)
        char ber_str[32];
        if (eponMgr_statsData_calculate_ber(link_stats, ber_str, sizeof(ber_str)) > 0) {
            rbusValue_SetString(value, ber_str);
        } else {
            rbusValue_SetString(value, "0.0");
        }
    }
    else if (strstr(param_name, "RangingResyncs")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->ranging_resyncs);
    }
    else if (strstr(param_name, "MACResets")) {
        rbusValue_SetUInt32(value, (uint32_t)link_stats->mac_resets);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Statistics SET handler (Stats.Reset only)
 * 
 * Per BBF TR-181 v2.18: Setting Stats.Reset to true resets all stats counters.
 */
static rbusError_t stats_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);

    EPONMGR_LOG_DEBUG("TR-181 SET: %s\n", param_name);

    if (strstr(param_name, ".Reset")) {
        bool reset = rbusValue_GetBoolean(value);
        if (reset) {
            /* Validate context is initialised, then release lock before HAL call.
             * Holding the mutex across epon_hal_clear_stats() is unnecessary and
             * would block other threads (e.g. stats poller) for the duration. */
            eponMgr_data_t *eponData = eponMgr_data_lock();
            if (!eponData) {
                EPONMGR_LOG_ERROR("Stats reset: data context not initialised\n");
                return RBUS_ERROR_BUS_ERROR;
            }
            eponMgr_data_unlock();

            epon_hal_return_t rc = eponMgr_data_clear_stats(eponData);
            if (rc != EPON_HAL_SUCCESS) {
                return RBUS_ERROR_BUS_ERROR;
            }

            EPONMGR_LOG_INFO("Statistics counters reset via TR-181 Stats.Reset\n");
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_INVALID_INPUT;
}

/**
 * @brief RBUS method handler for X_RDK_EPON.Reset()
 *
 * Triggers an ONU re-registration by calling epon_hal_reset_onu().
 * The ONU deregisters and restarts MPCP discovery and OAM negotiation.
 */
static rbusError_t epon_reset_method_handler(rbusHandle_t handle, char const* methodName,
                                             rbusObject_t inParams, rbusObject_t outParams,
                                             rbusMethodAsyncHandle_t asyncHandle) {
    (void)handle;
    (void)inParams;
    (void)outParams;
    (void)asyncHandle;

    EPONMGR_LOG_INFO("RBUS method invoked: %s\n", methodName);

    /* Validate context, then release lock before the HAL call.
     * epon_hal_reset_onu() triggers async HAL callbacks on a separate thread;
     * those callbacks acquire the same mutex via the controller, so holding it
     * here would deadlock. */
    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        EPONMGR_LOG_ERROR("Reset: data context not initialised\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    eponMgr_data_unlock();

    epon_hal_return_t rc = eponMgr_data_reset_onu(eponData);

    if (rc != EPON_HAL_SUCCESS) {
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief RBUS method handler for X_RDK_EPON.FactoryReset()
 *
 * Resets EPON HAL configuration to factory defaults by calling
 * epon_hal_factory_reset(). Clears all custom settings and statistics.
 * The ONU must be reconfigured and re-initialized after this operation.
 */
static rbusError_t epon_factory_reset_method_handler(rbusHandle_t handle, char const* methodName,
                                                     rbusObject_t inParams, rbusObject_t outParams,
                                                     rbusMethodAsyncHandle_t asyncHandle) {
    (void)handle;
    (void)inParams;
    (void)outParams;
    (void)asyncHandle;

    EPONMGR_LOG_INFO("RBUS method invoked: %s\n", methodName);

    /* Validate context, then release lock before the HAL call.
     * epon_hal_factory_reset() may trigger HAL callbacks on a separate thread;
     * holding the mutex here would deadlock against the callback path. */
    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        EPONMGR_LOG_ERROR("FactoryReset: data context not initialised\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    eponMgr_data_unlock();

    epon_hal_return_t rc = eponMgr_data_factory_reset(eponData);

    if (rc != EPON_HAL_SUCCESS) {
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Transceiver GET handler
 * 
 * Handles: Temperature, SupplyVoltage, BiasCurrent, and optical thresholds
 * (moved from BBF-deleted standard path per TR-181 v2.18)
 */
static rbusError_t transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get transceiver stats from HAL wrapper (with caching)
    const epon_hal_transceiver_stats_t* trans_stats = eponMgr_data_get_transceiver_stats(eponData);
    if (!trans_stats) {
        EPONMGR_LOG_ERROR("Failed to get transceiver stats\n");
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    /* All transceiver values scaled ×1000 to int32 (BBF TR-181 has no float/double) */
    if (strstr(param_name, "Temperature")) {
        int32_t temp = (int32_t)(trans_stats->temperature * 1000.0f);  /* m°C (millidegrees Celsius) */
        rbusValue_SetInt32(value, temp);
    }
    else if (strstr(param_name, "SupplyVoltage")) {
        int32_t voltage = (int32_t)(trans_stats->supply_voltage * 1000.0f);  /* mV (millivolts) */
        rbusValue_SetInt32(value, voltage);
    }
    else if (strstr(param_name, "BiasCurrent")) {
        int32_t current = (int32_t)(trans_stats->bias_current * 1000.0f);  /* µA (microamps) */
        rbusValue_SetInt32(value, current);
    }
    else if (strstr(param_name, "LowerOpticalThreshold")) {
        int32_t threshold = (int32_t)(trans_stats->lower_optical_threshold * 1000.0f);  /* Dbm1000 */
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "UpperOpticalThreshold")) {
        int32_t threshold = (int32_t)(trans_stats->upper_optical_threshold * 1000.0f);  /* Dbm1000 */
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "LowerTransmitPowerThreshold")) {
        int32_t threshold = (int32_t)(trans_stats->lower_transmit_power_threshold * 1000.0f);  /* Dbm1000 */
        rbusValue_SetInt32(value, threshold);
    }
    else if (strstr(param_name, "UpperTransmitPowerThreshold")) {
        int32_t threshold = (int32_t)(trans_stats->upper_transmit_power_threshold * 1000.0f);  /* Dbm1000 */
        rbusValue_SetInt32(value, threshold);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    if (strstr(param_name, "OperationalMode")) {
        // Get link info from HAL wrapper (with caching)
        const epon_hal_link_info_t* link_info = eponMgr_data_get_link_info(eponData);
        if (link_info) {
            rbusValue_SetString(value, link_info->mode);
        } else {
            rbusValue_SetString(value, "1G-EPON");  // Default
        }
    }
    else if (strstr(param_name, "EncryptionMode")) {
        const epon_hal_link_info_t* link_info = eponMgr_data_get_link_info(eponData);
        
        if (link_info) {
            const char *enc_str = encryption_mode_to_string(link_info->encryption);
            rbusValue_SetString(value, enc_str);
        } else {
            rbusValue_SetString(value, "None");
        }
    }
    else if (strstr(param_name, "ONUStatus")) {
        if (eponData->onu_state) {
            pthread_mutex_lock(&eponData->onu_state->mutex);
            const char *status_str = onu_status_to_string(eponData->onu_state->current_status);
            pthread_mutex_unlock(&eponData->onu_state->mutex);
            rbusValue_SetString(value, status_str);
        } else {
            rbusValue_SetString(value, "Unregistered");
        }
    }
    else if (strstr(param_name, "DPoESupported")) {
        rbusValue_SetBoolean(value, eponData->hal_config.dpoe_supported);
    }
    else if (strstr(param_name, "MaxLLIDSupported")) {
        pthread_mutex_lock(&eponData->mutex);
        uint32_t max_llid = eponData->llid_list.max_llid_count;
        pthread_mutex_unlock(&eponData->mutex);
        rbusValue_SetUInt32(value, max_llid);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get manufacturer info from HAL wrapper
    const epon_onu_manufacturer_info_t* mfr_info = eponMgr_data_get_onu_manufacturer_info(eponData);
    
    if (!mfr_info) {
        EPONMGR_LOG_ERROR("Failed to get manufacturer info\n");
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, ".Manufacturer")) {
        rbusValue_SetString(value, mfr_info->manufacturer);
    }
    else if (strstr(param_name, "ModelNumber")) {
        rbusValue_SetString(value, mfr_info->model_number);
    }
    else if (strstr(param_name, "HardwareVersion")) {
        rbusValue_SetString(value, mfr_info->hardware_version);
    }
    else if (strstr(param_name, "SoftwareVersion")) {
        rbusValue_SetString(value, mfr_info->software_version);
    }
    else if (strstr(param_name, "SerialNumber")) {
        rbusValue_SetString(value, mfr_info->serial_number);
    }
    else if (strstr(param_name, "VendorOUI")) {
        char oui_str[16];
        snprintf(oui_str, sizeof(oui_str), "%02X%02X%02X", 
                 mfr_info->vendor_oui[0], mfr_info->vendor_oui[1], mfr_info->vendor_oui[2]);
        rbusValue_SetString(value, oui_str);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) return RBUS_ERROR_BUS_ERROR;

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    // Get OLT info from HAL wrapper
    const epon_olt_info_t* olt_info = eponMgr_data_get_olt_info(eponData);
    
    if (!olt_info) {
        EPONMGR_LOG_ERROR("Failed to get OLT info\n");
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, "MACAddress")) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 olt_info->mac_address[0], olt_info->mac_address[1], olt_info->mac_address[2],
                 olt_info->mac_address[3], olt_info->mac_address[4], olt_info->mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strstr(param_name, "VendorOUI")) {
        char oui_str[16];
        snprintf(oui_str, sizeof(oui_str), "%02X%02X%02X",
                 olt_info->vendor_oui[0], olt_info->vendor_oui[1], olt_info->vendor_oui[2]);
        rbusValue_SetString(value, oui_str);
    }
    else if (strstr(param_name, "VendorSpecificInfo")) {
        rbusValue_SetString(value, "");  // Not available in current HAL
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
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

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    int ret;
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    /* Sync LLID data from HAL to ensure cache is current
     * Safe now that sync functions don't call back into data APIs */
    const epon_llid_list_t* llid_list = eponMgr_data_get_llid_info(eponData);
    if (!llid_list) {
        EPONMGR_LOG_WARN("Failed to sync LLID info from HAL, using cached data\n");
        /* Continue with cached data */
    }

    /* Ensure all LLID instances are registered after syncing fresh data */
    eponMgr_tr181_sync_llid_table();

    // Handle count parameter
    if (strstr(param_name, "LLIDNumberOfEntries")) {
        uint32_t count = eponMgr_data_get_llid_count(eponData);
        rbusValue_SetUInt32(value, count);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }

    // Handle table instance parameters (LLID.{i}.*)
    // Extract instance number from parameter name
    const char *llid_prefix = ".X_RDK_EPON.LLID.";
    const char *instance_start = strstr(param_name, llid_prefix);
    if (!instance_start) {
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_INVALID_INPUT;
    }

    instance_start += strlen(llid_prefix);
    uint32_t instance = 0;
    sscanf(instance_start, "%u", &instance);

    if (instance == 0 || instance > 32) {
        EPONMGR_LOG_ERROR("Invalid LLID instance: %u\n", instance);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Get LLID info from list (instance is 1-based)
    epon_llid_info_t llid_info;
    ret = eponMgr_data_get_llid_at_index(eponData, instance - 1, &llid_info);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("LLID instance %u not found\n", instance);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    // Determine which LLID parameter is requested
    if (strcmp(param_name + strlen(param_name) - strlen(".LLID"), ".LLID") == 0) {
        rbusValue_SetUInt32(value, llid_info.llid_value);
    }
    else if (strcmp(param_name + strlen(param_name) - strlen(".Status"), ".Status") == 0) {
        const char *status_str = llid_state_to_string(llid_info.state);
        rbusValue_SetString(value, status_str);
    }
    else if (strcmp(param_name + strlen(param_name) - strlen(".MACAddress"), ".MACAddress") == 0) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 llid_info.local_mac_address[0], llid_info.local_mac_address[1],
                 llid_info.local_mac_address[2], llid_info.local_mac_address[3],
                 llid_info.local_mac_address[4], llid_info.local_mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strcmp(param_name + strlen(param_name) - strlen(".Mode"), ".Mode") == 0) {
        const char *mode_str = (llid_info.mode == EPON_LLID_MODE_UNICAST) ? "Unicast" : "Broadcast";
        rbusValue_SetString(value, mode_str);
    }
    else if (strcmp(param_name + strlen(param_name) - strlen(".EncryptionEnabled"), ".EncryptionEnabled") == 0) {
        rbusValue_SetBoolean(value, llid_info.encryption_enabled);
    }
    else if (strcmp(param_name + strlen(param_name) - strlen(".ForwardingState"), ".ForwardingState") == 0) {
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
    eponMgr_data_unlock();
    return RBUS_ERROR_SUCCESS;
}

/* ============================================================================
 * Phase 7.2: DPOE/CPE Dynamic Table Handler
 * ============================================================================ */

/**
 * @brief CPE table handler
 * 
 * Handles: DPOE stats, CPENumberOfEntries, CPE.{i}.* parameters
 */
static rbusError_t cpe_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return RBUS_ERROR_BUS_ERROR;
    }

    const char* param_name = rbusProperty_GetName(property);
    rbusValue_t value;
    rbusValue_Init(&value);
    int ret;
    
    EPONMGR_LOG_DEBUG("TR-181 GET: %s\n", param_name);

    /* Sync CPE data from HAL to ensure cache is current
     * Safe now that sync functions don't call back into data APIs */
    const dpoe_cpe_mac_table_t* cpe_table = eponMgr_data_get_cpe_mac_table(eponData);
    if (!cpe_table) {
        EPONMGR_LOG_WARN("Failed to sync CPE MAC table from HAL, using cached data\n");
        /* Continue with cached data */
    }

    /* Ensure all CPE instances are registered after syncing fresh data */
    eponMgr_tr181_sync_cpe_table();

    // Handle DPOE statistics
    if (strstr(param_name, "MaxCPECount")) {
        pthread_mutex_lock(&eponData->mutex);
        uint32_t max_cpe = eponData->cpe_table.max_cpe;
        pthread_mutex_unlock(&eponData->mutex);
        rbusValue_SetUInt32(value, max_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "StaticCPECount")) {
        pthread_mutex_lock(&eponData->mutex);
        uint32_t static_cpe = eponData->cpe_table.static_cpe_count;
        pthread_mutex_unlock(&eponData->mutex);
        rbusValue_SetUInt32(value, static_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "DynamicCPECount")) {
        pthread_mutex_lock(&eponData->mutex);
        uint32_t dynamic_cpe = eponData->cpe_table.dynamic_cpe_count;
        pthread_mutex_unlock(&eponData->mutex);
        rbusValue_SetUInt32(value, dynamic_cpe);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }
    else if (strstr(param_name, "CPENumberOfEntries")) {
        uint32_t count = eponMgr_data_get_cpe_count(eponData);
        rbusValue_SetUInt32(value, count);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }

    // Handle table instance parameters (CPE.{i}.*)
    const char *cpe_prefix = ".DPOE.CPE.";
    const char *instance_start = strstr(param_name, cpe_prefix);
    if (!instance_start) {
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_INVALID_INPUT;
    }

    instance_start += strlen(cpe_prefix);
    uint32_t instance = 0;
    sscanf(instance_start, "%u", &instance);

    if (instance == 0 || instance > 256) {
        EPONMGR_LOG_ERROR("Invalid CPE instance: %u\n", instance);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Get CPE entry from list (instance is 1-based)
    dpoe_cpe_mac_entry_t cpe_entry;
    ret = eponMgr_data_get_cpe_at_index(eponData, instance - 1, &cpe_entry);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("CPE instance %u not found\n", instance);
        rbusValue_Release(value);
        eponMgr_data_unlock();
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    // Determine which CPE parameter is requested
    if (strcmp(param_name + strlen(param_name) - 11, ".MACAddress") == 0) {
        char mac_str[24];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 cpe_entry.mac_address[0], cpe_entry.mac_address[1],
                 cpe_entry.mac_address[2], cpe_entry.mac_address[3],
                 cpe_entry.mac_address[4], cpe_entry.mac_address[5]);
        rbusValue_SetString(value, mac_str);
    }
    else if (strcmp(param_name + strlen(param_name) - 8, ".AgeTime") == 0) {
        rbusValue_SetUInt32(value, cpe_entry.age_time);
    }
    else if (strcmp(param_name + strlen(param_name) - 5, ".Type") == 0) {
        const char *type_str = (cpe_entry.type == DPOE_CPE_MAC_STATIC) ? "Static" : "Dynamic";
        rbusValue_SetString(value, type_str);
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
    return RBUS_ERROR_SUCCESS;
}

/* ============================================================================
 * VEIP Interface Dynamic Table Support (Phase 7.3)
 * ========================================================================== */

/**
 * @brief Synchronize VEIP Interface table registrations with current interface list
 * 
 * Compares current RBUS registrations with actual interface list state
 * and registers/unregisters instances as needed. Should be called when
 * interface list changes (add/remove/status change events).
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Acquires data lock internally, safe to call from any thread
 * @note Automatically registers new VEIP instances in TR-181 table
 * @note Stores interface name in instance table for lookup
 */
int eponMgr_tr181_sync_veip_table(void) {
    if (!g_rbus_handle) {
        return -1;
    }

    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return -1;
    }

    uint32_t interface_count = eponMgr_data_get_interface_count(eponData);
    
    /* Register VEIP instances (1-based indexing) */
    for (uint32_t i = 0; i < interface_count && i < MAX_VEIP_INSTANCES; i++) {
        uint32_t instance = i + 1;
        if (!g_veip_instances[instance - 1].registered) {
            epon_onu_interface_info_t interface_info;
            if (eponMgr_data_get_interface_at_index(eponData, i, &interface_info) == 0) {
                /* Register this VEIP instance */
                rbusError_t rc = rbusTable_registerRow(g_rbus_handle, TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.", instance, NULL);
                if (rc == RBUS_ERROR_SUCCESS) {
                    g_veip_instances[instance - 1].instance = instance;
                    strncpy(g_veip_instances[instance - 1].name, interface_info.name, 
                            sizeof(g_veip_instances[instance - 1].name) - 1);
                    g_veip_instances[instance - 1].name[sizeof(g_veip_instances[instance - 1].name) - 1] = '\0';
                    g_veip_instances[instance - 1].registered = true;
                    EPONMGR_LOG_INFO("Registered VEIP table row %s%u on-demand\n", TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.",instance);
                }
            }
        }
    }

    eponMgr_data_unlock();
    return 0;
}

/**
 * @brief RBUS GET handler for VEIP Interface table parameters
 */
static rbusError_t veip_table_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;
    
    const char *param_name = rbusProperty_GetName(property);
    if (!param_name) return RBUS_ERROR_INVALID_INPUT;
    
    /* Handle VEIP_InterfaceNumberOfEntries */
    if (strstr(param_name, "VEIP_InterfaceNumberOfEntries")) {
        eponMgr_data_t *eponData = eponMgr_data_lock();
        if (!eponData) {
            return RBUS_ERROR_BUS_ERROR;
        }
        
        uint32_t interface_count = eponData->interface_list.interface_count;
        
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetUInt32(value, interface_count);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        
        eponMgr_data_unlock();
        return RBUS_ERROR_SUCCESS;
    }
    
    /* Handle row parameters (.Name, .Status) */
    uint32_t instance = 0;
    if (sscanf(param_name, TR181_BASE_PATH ".X_RDK_EPON.VEIP_Interface.%u", &instance) != 1) {
        EPONMGR_LOG_ERROR("Failed to parse VEIP instance from: %s\n", param_name);
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if (instance == 0 || instance > MAX_VEIP_INSTANCES) {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    uint32_t idx = instance - 1;
    
    if (!g_veip_instances[idx].registered) {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    const char *iface_name = g_veip_instances[idx].name;
    
    eponMgr_data_t *eponData = eponMgr_data_lock();
    if (!eponData) {
        return RBUS_ERROR_BUS_ERROR;
    }
    
    epon_onu_interface_info_t info;
    if (eponMgr_data_get_interface_by_name(eponData, iface_name, &info) != 0) {
        eponMgr_data_unlock();
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    
    rbusValue_t value;
    rbusValue_Init(&value);
    
    if (strstr(param_name, ".Name")) {
        rbusValue_SetString(value, info.name);
    }
    else if (strstr(param_name, ".Status")) {
        const char *status_str = (info.status == EPON_ONU_INTF_STATUS_LINK_UP) ? "Up" : "Down";
        rbusValue_SetString(value, status_str);
    }
    
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    eponMgr_data_unlock();
    return RBUS_ERROR_SUCCESS;
}
/**
 * @brief GET handler for stats poller configuration parameters
 */
static rbusError_t stats_poller_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;
    
    rbusError_t result = RBUS_ERROR_SUCCESS;
    
    const char *param_name = rbusProperty_GetName(property);
    if (!param_name) {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    EPONMGR_LOG_DEBUG("GET: %s\n", param_name);
    
    rbusValue_t value;
    rbusValue_Init(&value);
    
    /* Lock and get persistent config which was loaded from PSM at startup */
    const eponMgr_persistence_t *config = eponMgr_controller_lock_persistence_config();
    if (!config) {
        EPONMGR_LOG_WARN("Persistence config not available\n");
        rbusValue_Release(value);
        eponMgr_controller_unlock_persistence_config();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, ".Enable")) {
        /* Read from persistent config which was loaded from PSM */
        bool enabled = config->stats_poller_enabled;
        EPONMGR_LOG_DEBUG("Stats poller enabled from config: %s\n", 
                        enabled ? "true" : "false");
        rbusValue_SetBoolean(value, enabled);
    }
    else if (strstr(param_name, ".PollingInterval")) {
        /* Read from persistent config which was loaded from PSM */
        uint32_t interval = config->stats_poller_interval_seconds;
        EPONMGR_LOG_DEBUG("Stats poller interval from config: %u seconds\n", interval);
        rbusValue_SetUInt32(value, interval);
    }
    else {
        result = RBUS_ERROR_INVALID_INPUT;
    }
    
    if (result == RBUS_ERROR_SUCCESS) {
        rbusProperty_SetValue(property, value);
    }
    
    rbusValue_Release(value);
    
    /* Unlock after we're done reading config */
    eponMgr_controller_unlock_persistence_config();
    
    return result;
}

/**
 * @brief SET handler for stats poller configuration parameters
 */
static rbusError_t stats_poller_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts) {
    (void)handle;
    (void)opts;
    
    rbusError_t result = RBUS_ERROR_SUCCESS;
    
    const char *param_name = rbusProperty_GetName(property);
    if (!param_name) {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    rbusValue_t value = rbusProperty_GetValue(property);
    if (!value) {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    EPONMGR_LOG_INFO("SET: %s\n", param_name);
    
    /* Get stats poller from controller */
    eponMgr_stats_poller_t *poller = (eponMgr_stats_poller_t *)eponMgr_controller_get_stats_poller();
    if (!poller) {
        EPONMGR_LOG_ERROR("Stats poller not available\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    if (strstr(param_name, ".Enable")) {
        bool enabled = rbusValue_GetBoolean(value);
        bool is_running = eponMgr_stats_poller_is_running(poller);
        
        /* Handle dynamic start/stop */
        if (enabled && !is_running) {
            /* Start the thread and set enabled flag (persistence done internally) */
            if (eponMgr_stats_poller_set_enabled(poller, true) != 0) {
                EPONMGR_LOG_ERROR("Failed to set stats poller enabled flag\n");
                result = RBUS_ERROR_BUS_ERROR;
            } else if (eponMgr_stats_poller_start(poller) != 0) {
                EPONMGR_LOG_ERROR("Failed to start stats poller thread\n");
                result = RBUS_ERROR_BUS_ERROR;
            } else {
                EPONMGR_LOG_INFO("Stats poller thread started via TR-181\n");
            }
        } else if (!enabled && is_running) {
            /* Stop the thread and set enabled flag (persistence done internally) */
            eponMgr_stats_poller_stop(poller);
            if (eponMgr_stats_poller_set_enabled(poller, false) != 0) {
                EPONMGR_LOG_ERROR("Failed to set stats poller enabled flag\n");
                result = RBUS_ERROR_BUS_ERROR;
            } else {
                EPONMGR_LOG_INFO("Stats poller thread stopped via TR-181\n");
            }
        } else {
            /* Thread state already matches, just update the flag (persistence done internally) */
            if (eponMgr_stats_poller_set_enabled(poller, enabled) != 0) {
                EPONMGR_LOG_ERROR("Failed to set stats poller enabled flag\n");
                result = RBUS_ERROR_BUS_ERROR;
            } else {
                EPONMGR_LOG_INFO("Stats poller %s flag updated via TR-181\n", enabled ? "enabled" : "disabled");
            }
        }
    }
    else if (strstr(param_name, ".PollingInterval")) {
        uint32_t interval = rbusValue_GetUInt32(value);
        
        /* Validate interval (60-3600 seconds) */
        if (interval < 60 || interval > 3600) {
            EPONMGR_LOG_ERROR("Invalid polling interval: %u (must be 60-3600)\n", interval);
            result = RBUS_ERROR_INVALID_INPUT;
        } else if (eponMgr_stats_poller_set_interval(poller, interval) != 0) {
            /* Apply runtime change (persistence done internally) */
            EPONMGR_LOG_ERROR("Failed to set stats poller interval\n");
            result = RBUS_ERROR_BUS_ERROR;
        } else {
            EPONMGR_LOG_INFO("Stats poller interval set to %u seconds via TR-181\n", interval);
        }
    }
    else {
        result = RBUS_ERROR_INVALID_INPUT;
    }
    
    eponMgr_controller_unlock_stats_poller();
    return result;
}
