/**
 * @file epon_hal_mock.c
 * @brief Mock EPON HAL implementation for testing
 */

#include "epon_hal_mock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Mock internal state */
static bool g_initialized = false;
static epon_hal_config_t g_config = {0};
static epon_hal_link_stats_t g_link_stats = {0};
static epon_hal_transceiver_stats_t g_transceiver_stats = {0};
static epon_onu_manufacturer_info_t g_manufacturer_info = {0};
static epon_hal_link_info_t g_link_info = {0};
static epon_interface_list_t g_interface_list = {0};
static epon_olt_info_t g_olt_info = {0};
static epon_llid_list_t g_llid_list = {0};

/* Initialize default mock data */
static void init_default_data(void) {
    /* Default link stats */
    g_link_stats.struct_size = sizeof(g_link_stats);
    g_link_stats.packets_sent = 1000;
    g_link_stats.packets_received = 2000;
    g_link_stats.bytes_sent = 64000;
    g_link_stats.bytes_received = 128000;
    g_link_stats.max_bit_rate = 1000; /* 1Gbps */
    
    /* Default transceiver stats */
    g_transceiver_stats.struct_size = sizeof(g_transceiver_stats);
    g_transceiver_stats.transmit_optical_level = -2.5f;
    g_transceiver_stats.optical_signal_level = -15.0f;
    g_transceiver_stats.lower_optical_threshold = -25.0f;
    g_transceiver_stats.upper_optical_threshold = -5.0f;
    g_transceiver_stats.bias_current = 35.5f;
    g_transceiver_stats.temperature = 45.0f;
    g_transceiver_stats.supply_voltage = 3.3f;
    
    /* Default manufacturer info */
    g_manufacturer_info.struct_size = sizeof(g_manufacturer_info);
    strncpy(g_manufacturer_info.manufacturer, "Mock EPON Vendor", EPON_HAL_MANUFACTURER_LEN - 1);
    strncpy(g_manufacturer_info.model_number, "MOCK-ONU-1G", EPON_HAL_MODEL_NUMBER_LEN - 1);
    strncpy(g_manufacturer_info.hardware_version, "1.0", EPON_HAL_HW_VERSION_LEN - 1);
    strncpy(g_manufacturer_info.software_version, "1.0.0", EPON_HAL_SW_VERSION_LEN - 1);
    strncpy(g_manufacturer_info.serial_number, "MOCK1234567890", EPON_HAL_SERIAL_NUMBER_LEN - 1);
    g_manufacturer_info.vendor_oui[0] = 0x00;
    g_manufacturer_info.vendor_oui[1] = 0x11;
    g_manufacturer_info.vendor_oui[2] = 0x22;
    
    /* Default link info */
    strncpy(g_link_info.mode, "1G-EPON", EPON_HAL_MODE_LEN - 1);
    g_link_info.encryption = EPON_ENCRYPTION_MODE_AES_128;
    
    /* Default OLT info */
    g_olt_info.struct_size = sizeof(g_olt_info);
    g_olt_info.mac_address[0] = 0xAA;
    g_olt_info.mac_address[1] = 0xBB;
    g_olt_info.mac_address[2] = 0xCC;
    g_olt_info.mac_address[3] = 0xDD;
    g_olt_info.mac_address[4] = 0xEE;
    g_olt_info.mac_address[5] = 0xFF;
    g_olt_info.vendor_oui[0] = 0x00;
    g_olt_info.vendor_oui[1] = 0x01;
    g_olt_info.vendor_oui[2] = 0x02;
    
    /* Default interface list - single interface */
    g_interface_list.interface_count = 1;
    strncpy(g_interface_list.interface[0].name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    g_interface_list.interface[0].status = EPON_ONU_INTF_STATUS_LINK_UP;
    
    /* Default LLID list */
    g_llid_list.max_llid_count = 8;
    g_llid_list.llid_count = 1;
    g_llid_list.llid_list = NULL; /* Will be allocated in get_llid_info */
}

/**
 * HAL API implementations
 */

uint32_t epon_hal_get_version(void) {
    return EPON_HAL_API_VERSION;
}

int epon_hal_init(const epon_hal_config_t *config) {
    if (!config) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (config->struct_size != sizeof(epon_hal_config_t)) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    /* Save configuration */
    memcpy(&g_config, config, sizeof(epon_hal_config_t));
    
    /* Initialize default data */
    init_default_data();
    
    g_initialized = true;
    
    printf("EPON HAL Mock: Initialized\n");
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_link_stats(epon_hal_link_stats_t *stats) {
    if (!stats) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (stats->struct_size != sizeof(epon_hal_link_stats_t)) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(stats, &g_link_stats, sizeof(epon_hal_link_stats_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_transceiver_stats(epon_hal_transceiver_stats_t *stats) {
    if (!stats) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (stats->struct_size != sizeof(epon_hal_transceiver_stats_t)) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(stats, &g_transceiver_stats, sizeof(epon_hal_transceiver_stats_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_llid_info(epon_llid_list_t *llid_list) {
    if (!llid_list) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Allocate LLID array */
    llid_list->max_llid_count = g_llid_list.max_llid_count;
    llid_list->llid_count = g_llid_list.llid_count;
    
    if (llid_list->llid_count > 0) {
        llid_list->llid_list = (epon_llid_info_t *)malloc(llid_list->llid_count * sizeof(epon_llid_info_t));
        if (!llid_list->llid_list) {
            return EPON_HAL_ERROR_MEMORY;
        }
        
        /* Fill default LLID info */
        llid_list->llid_list[0].llid_value = 1;
        llid_list->llid_list[0].mode = EPON_LLID_MODE_UNICAST;
        llid_list->llid_list[0].state = EPON_LLID_STATE_REGISTERED;
        llid_list->llid_list[0].forwarding_state = EPON_LLID_FORWARDING_ENABLED;
        llid_list->llid_list[0].encryption_enabled = true;
        llid_list->llid_list[0].local_mac_address[0] = 0x00;
        llid_list->llid_list[0].local_mac_address[1] = 0x11;
        llid_list->llid_list[0].local_mac_address[2] = 0x22;
        llid_list->llid_list[0].local_mac_address[3] = 0x33;
        llid_list->llid_list[0].local_mac_address[4] = 0x44;
        llid_list->llid_list[0].local_mac_address[5] = 0x55;
    }
    
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_manufacturer_info(epon_onu_manufacturer_info_t *info) {
    if (!info) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (info->struct_size != sizeof(epon_onu_manufacturer_info_t)) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(info, &g_manufacturer_info, sizeof(epon_onu_manufacturer_info_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_clear_stats(void) {
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Reset all counters to 0 */
    g_link_stats.packets_sent = 0;
    g_link_stats.packets_received = 0;
    g_link_stats.bytes_sent = 0;
    g_link_stats.bytes_received = 0;
    g_link_stats.errors_sent = 0;
    g_link_stats.errors_received = 0;
    g_link_stats.discard_packets_sent = 0;
    g_link_stats.discard_packets_received = 0;
    g_link_stats.fec_corrected = 0;
    g_link_stats.fec_uncorrectable = 0;
    g_link_stats.broadcast_packets_sent = 0;
    g_link_stats.broadcast_packets_received = 0;
    g_link_stats.multicast_packets_sent = 0;
    g_link_stats.multicast_packets_received = 0;
    g_link_stats.ranging_resyncs = 0;
    g_link_stats.mac_resets = 0;
    
    printf("EPON HAL Mock: Statistics cleared\n");
    return EPON_HAL_SUCCESS;
}

int epon_hal_reset_onu(void) {
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    printf("EPON HAL Mock: ONU reset triggered\n");
    
    /* In real implementation, this would trigger hardware reset */
    /* In mock, just trigger status callbacks */
    if (g_config.status_callback) {
        g_config.status_callback(EPON_ONU_STATUS_DEREGISTRATION);
        g_config.status_callback(EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED);
        g_config.status_callback(EPON_ONU_STATUS_REGISTRATION);
    }
    
    return EPON_HAL_SUCCESS;
}

int epon_hal_factory_reset(void) {
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    printf("EPON HAL Mock: Factory reset triggered\n");
    
    /* Reset to defaults */
    init_default_data();
    
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_link_info(epon_hal_link_info_t *info) {
    if (!info) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(info, &g_link_info, sizeof(epon_hal_link_info_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_interface_list(epon_interface_list_t *if_list) {
    if (!if_list) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(if_list, &g_interface_list, sizeof(epon_interface_list_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_get_olt_info(epon_olt_info_t *olt_info) {
    if (!olt_info) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (olt_info->struct_size != sizeof(epon_olt_info_t)) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(olt_info, &g_olt_info, sizeof(epon_olt_info_t));
    return EPON_HAL_SUCCESS;
}

int epon_hal_set_oam_log_mask(uint32_t oam_log_mask) {
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    printf("EPON HAL Mock: OAM log mask set to 0x%08X\n", oam_log_mask);
    return EPON_HAL_SUCCESS;
}

int dpoe_hal_get_cpe_mac_table(dpoe_cpe_mac_table_t *cpe_table) {
    if (!cpe_table) {
        return EPON_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return EPON_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Return empty table for now */
    cpe_table->max_cpe = 32;
    cpe_table->static_cpe_count = 0;
    cpe_table->dynamic_cpe_count = 0;
    cpe_table->cpe_list = NULL;
    
    return EPON_HAL_SUCCESS;
}

/**
 * Test-only trigger functions
 */

void epon_hal_mock_trigger_status(epon_onu_status_t status) {
    if (g_initialized && g_config.status_callback) {
        printf("EPON HAL Mock: Triggering status callback - status=%d\n", status);
        g_config.status_callback(status);
    }
}

void epon_hal_mock_trigger_alarm(epon_hal_alarm_t alarm, bool is_active) {
    if (g_initialized && g_config.alarm_callback) {
        printf("EPON HAL Mock: Triggering alarm callback - alarm=%d, active=%d\n", alarm, is_active);
        g_config.alarm_callback(alarm, is_active);
    }
}

void epon_hal_mock_trigger_interface_status(const char *interface_name, 
                                           epon_interface_link_status_t status) {
    if (g_initialized && g_config.interface_status_callback) {
        epon_onu_interface_info_t info;
        strncpy(info.name, interface_name, EPON_HAL_INTERFACE_NAME_LEN - 1);
        info.name[EPON_HAL_INTERFACE_NAME_LEN - 1] = '\0';
        info.status = status;
        
        printf("EPON HAL Mock: Triggering interface status callback - interface=%s, status=%d\n", 
               interface_name, status);
        g_config.interface_status_callback(info);
    }
}

void epon_hal_mock_set_link_stats(const epon_hal_link_stats_t *stats) {
    if (stats) {
        memcpy(&g_link_stats, stats, sizeof(epon_hal_link_stats_t));
    }
}

void epon_hal_mock_set_transceiver_stats(const epon_hal_transceiver_stats_t *stats) {
    if (stats) {
        memcpy(&g_transceiver_stats, stats, sizeof(epon_hal_transceiver_stats_t));
    }
}
