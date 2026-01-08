
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
 * @file epon_hal_mock.c
 * @brief Mock EPON HAL implementation for testing
 */

#include "epon_hal_mock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define EPON_HAL_MOCK_SOCKET_PATH "/tmp/epon_hal_mock.sock"
#define MAX_COMMAND_LEN 256

/* Mock internal state */
static bool g_initialized = false;
static pthread_t g_control_thread;
static int g_control_socket = -1;
static bool g_control_running = false;
static epon_hal_config_t g_config = {0};
static epon_hal_link_stats_t g_link_stats = {0};
static epon_hal_transceiver_stats_t g_transceiver_stats = {0};
static epon_onu_manufacturer_info_t g_manufacturer_info = {0};
static epon_hal_link_info_t g_link_info = {0};
static epon_interface_list_t g_interface_list = {0};
static epon_olt_info_t g_olt_info = {0};
static epon_llid_list_t g_llid_list = {0};
static epon_llid_info_t g_llid_entries[8] = {0};
static uint32_t g_llid_count = 0;
static dpoe_cpe_mac_entry_t g_cpe_entries[32] = {0};
static uint32_t g_static_cpe_count = 0;
static uint32_t g_dynamic_cpe_count = 0;

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
    
    /* Default interface list - use static array from structure */
    memset(&g_interface_list, 0, sizeof(g_interface_list));
    
    /* Add initial veip0 interface */
    strncpy(g_interface_list.interface[0].name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    g_interface_list.interface[0].name[EPON_HAL_INTERFACE_NAME_LEN - 1] = '\0';
    g_interface_list.interface[0].status = EPON_ONU_INTF_STATUS_LINK_UP;
    g_interface_list.interface_count = 1;
    
    /* Default LLID list - 4 LLIDs with various states */
    memset(g_llid_entries, 0, sizeof(g_llid_entries));
    
    /* LLID 1 - Unicast, Registered, Encryption enabled */
    g_llid_entries[0].llid_value = 1;
    g_llid_entries[0].mode = EPON_LLID_MODE_UNICAST;
    g_llid_entries[0].state = EPON_LLID_STATE_REGISTERED;
    g_llid_entries[0].forwarding_state = EPON_LLID_FORWARDING_ENABLED;
    g_llid_entries[0].encryption_enabled = true;
    g_llid_entries[0].local_mac_address[0] = 0x00;
    g_llid_entries[0].local_mac_address[1] = 0x11;
    g_llid_entries[0].local_mac_address[2] = 0x22;
    g_llid_entries[0].local_mac_address[3] = 0x33;
    g_llid_entries[0].local_mac_address[4] = 0x44;
    g_llid_entries[0].local_mac_address[5] = 0x55;
    
    /* LLID 2 - Multicast, Registered, Encryption disabled */
    g_llid_entries[1].llid_value = 32767; /* Multicast LLID */
    g_llid_entries[1].mode = EPON_LLID_MODE_BROADCAST;
    g_llid_entries[1].state = EPON_LLID_STATE_REGISTERED;
    g_llid_entries[1].forwarding_state = EPON_LLID_FORWARDING_ENABLED;
    g_llid_entries[1].encryption_enabled = false;
    g_llid_entries[1].local_mac_address[0] = 0xFF;
    g_llid_entries[1].local_mac_address[1] = 0xFF;
    g_llid_entries[1].local_mac_address[2] = 0xFF;
    g_llid_entries[1].local_mac_address[3] = 0xFF;
    g_llid_entries[1].local_mac_address[4] = 0xFF;
    g_llid_entries[1].local_mac_address[5] = 0xFF;
    
    /* LLID 3 - Unicast, Registered, In learning state */
    g_llid_entries[2].llid_value = 2;
    g_llid_entries[2].mode = EPON_LLID_MODE_UNICAST;
    g_llid_entries[2].state = EPON_LLID_STATE_REGISTERED;
    g_llid_entries[2].forwarding_state = EPON_LLID_FORWARDING_LEARNING;
    g_llid_entries[2].encryption_enabled = true;
    g_llid_entries[2].local_mac_address[0] = 0x00;
    g_llid_entries[2].local_mac_address[1] = 0xAA;
    g_llid_entries[2].local_mac_address[2] = 0xBB;
    g_llid_entries[2].local_mac_address[3] = 0xCC;
    g_llid_entries[2].local_mac_address[4] = 0xDD;
    g_llid_entries[2].local_mac_address[5] = 0xEE;
    
    /* LLID 4 - Unicast, Deregistering state */
    g_llid_entries[3].llid_value = 3;
    g_llid_entries[3].mode = EPON_LLID_MODE_UNICAST;
    g_llid_entries[3].state = EPON_LLID_STATE_DEREGISTERING;
    g_llid_entries[3].forwarding_state = EPON_LLID_FORWARDING_DISABLED;
    g_llid_entries[3].encryption_enabled = true;
    g_llid_entries[3].local_mac_address[0] = 0x00;
    g_llid_entries[3].local_mac_address[1] = 0x99;
    g_llid_entries[3].local_mac_address[2] = 0x88;
    g_llid_entries[3].local_mac_address[3] = 0x77;
    g_llid_entries[3].local_mac_address[4] = 0x66;
    g_llid_entries[3].local_mac_address[5] = 0x55;
    
    g_llid_count = 4;
    
    g_llid_list.max_llid_count = 8;
    g_llid_list.llid_count = g_llid_count;
    g_llid_list.llid_list = NULL; /* Will be allocated in get_llid_info */
    
    /* Default CPE list - 2 static, 2 dynamic */
    memset(g_cpe_entries, 0, sizeof(g_cpe_entries));
    
    /* Static CPE 1 */
    g_cpe_entries[0].mac_address[0] = 0x00;
    g_cpe_entries[0].mac_address[1] = 0x1A;
    g_cpe_entries[0].mac_address[2] = 0x2B;
    g_cpe_entries[0].mac_address[3] = 0x3C;
    g_cpe_entries[0].mac_address[4] = 0x4D;
    g_cpe_entries[0].mac_address[5] = 0x5E;
    g_cpe_entries[0].type = DPOE_CPE_MAC_STATIC;
    g_cpe_entries[0].age_time = 0; /* Static entries have 0 age time */
    
    /* Static CPE 2 */
    g_cpe_entries[1].mac_address[0] = 0x00;
    g_cpe_entries[1].mac_address[1] = 0x2B;
    g_cpe_entries[1].mac_address[2] = 0x3C;
    g_cpe_entries[1].mac_address[3] = 0x4D;
    g_cpe_entries[1].mac_address[4] = 0x5E;
    g_cpe_entries[1].mac_address[5] = 0x6F;
    g_cpe_entries[1].type = DPOE_CPE_MAC_STATIC;
    g_cpe_entries[1].age_time = 0;
    
    g_static_cpe_count = 2;
    
    /* Dynamic CPE 1 */
    g_cpe_entries[2].mac_address[0] = 0x00;
    g_cpe_entries[2].mac_address[1] = 0x3C;
    g_cpe_entries[2].mac_address[2] = 0x4D;
    g_cpe_entries[2].mac_address[3] = 0x5E;
    g_cpe_entries[2].mac_address[4] = 0x6F;
    g_cpe_entries[2].mac_address[5] = 0x70;
    g_cpe_entries[2].type = DPOE_CPE_MAC_DYNAMIC;
    g_cpe_entries[2].age_time = 3600; /* 1 hour */
    
    /* Dynamic CPE 2 */
    g_cpe_entries[3].mac_address[0] = 0x00;
    g_cpe_entries[3].mac_address[1] = 0x4D;
    g_cpe_entries[3].mac_address[2] = 0x5E;
    g_cpe_entries[3].mac_address[3] = 0x6F;
    g_cpe_entries[3].mac_address[4] = 0x70;
    g_cpe_entries[3].mac_address[5] = 0x81;
    g_cpe_entries[3].type = DPOE_CPE_MAC_DYNAMIC;
    g_cpe_entries[3].age_time = 7200; /* 2 hours */
    
    g_dynamic_cpe_count = 2;
}

/**
 * Control socket thread for inter-process communication
 */
static void process_command(const char *cmd) {
    char cmd_copy[MAX_COMMAND_LEN];
    strncpy(cmd_copy, cmd, MAX_COMMAND_LEN - 1);
    cmd_copy[MAX_COMMAND_LEN - 1] = '\0';
    
    /* Parse command: TYPE:value1:value2 */
    char *saveptr;
    char *type = strtok_r(cmd_copy, ":", &saveptr);
    if (!type) return;
    
    if (strcmp(type, "STATUS") == 0) {
        char *status_str = strtok_r(NULL, ":", &saveptr);
        if (!status_str) return;
        
        int status = atoi(status_str);
        printf("EPON HAL Mock: Processing STATUS command: %d\n", status);
        
        if (g_initialized && g_config.status_callback) {
            g_config.status_callback((epon_onu_status_t)status);
        }
    }
    else if (strcmp(type, "ALARM") == 0) {
        char *alarm_type_str = strtok_r(NULL, ":", &saveptr);
        char *alarm_value_str = strtok_r(NULL, ":", &saveptr);
        char *llid_str = strtok_r(NULL, ":", &saveptr);
        char *active_str = strtok_r(NULL, ":", &saveptr);
        if (!alarm_type_str || !alarm_value_str || !llid_str || !active_str) return;
        
        int alarm_type = atoi(alarm_type_str);
        uint32_t alarm_value = (uint32_t)atoi(alarm_value_str);
        uint16_t llid = (uint16_t)atoi(llid_str);
        int active = atoi(active_str);
        printf("EPON HAL Mock: Processing ALARM command: type=%d, value=%u, llid=%u, active=%d\n", 
               alarm_type, alarm_value, llid, active);
        
        if (g_initialized && g_config.alarm_callback) {
            epon_hal_mock_trigger_alarm((epon_alarm_type_t)alarm_type, alarm_value, llid, (bool)active);
        }
    }
    else if (strcmp(type, "INTERFACE") == 0) {
        char *if_name = strtok_r(NULL, ":", &saveptr);
        char *if_status = strtok_r(NULL, ":", &saveptr);
        if (!if_name || !if_status) return;
        
        int status = atoi(if_status);
        printf("EPON HAL Mock: Processing INTERFACE command: %s, status=%d\n", if_name, status);
        
        /* Update local mock data - find existing or add new interface */
        int found_index = -1;
        for (uint32_t i = 0; i < g_interface_list.interface_count; i++) {
            if (strncmp(g_interface_list.interface[i].name, if_name, EPON_HAL_INTERFACE_NAME_LEN) == 0) {
                found_index = i;
                break;
            }
        }
        
        if (found_index >= 0) {
            /* Update existing interface */
            g_interface_list.interface[found_index].status = (epon_interface_link_status_t)status;
            printf("EPON HAL Mock: Updated interface[%d].status to %d\n", found_index, status);
        } else if (g_interface_list.interface_count < EPON_HAL_MAX_INTERFACES) {
            /* Add new interface */
            uint32_t new_index = g_interface_list.interface_count;
            strncpy(g_interface_list.interface[new_index].name, if_name, EPON_HAL_INTERFACE_NAME_LEN - 1);
            g_interface_list.interface[new_index].name[EPON_HAL_INTERFACE_NAME_LEN - 1] = '\0';
            g_interface_list.interface[new_index].status = (epon_interface_link_status_t)status;
            g_interface_list.interface_count++;
            printf("EPON HAL Mock: Added new interface[%u] '%s' with status %d\n", new_index, if_name, status);
        } else {
            printf("EPON HAL Mock: Cannot add interface '%s' - max interfaces (%u) reached\n", if_name, EPON_HAL_MAX_INTERFACES);
            return;
        }
        
        if (g_initialized && g_config.interface_status_callback) {
            epon_onu_interface_info_t info;
            strncpy(info.name, if_name, EPON_HAL_INTERFACE_NAME_LEN - 1);
            info.name[EPON_HAL_INTERFACE_NAME_LEN - 1] = '\0';
            info.status = (epon_interface_link_status_t)status;
            g_config.interface_status_callback(info);
        }
    }
    else if (strcmp(type, "LINKSTATS") == 0) {
        char *field = strtok_r(NULL, ":", &saveptr);
        char *value = strtok_r(NULL, ":", &saveptr);
        if (!field || !value) return;
        
        printf("EPON HAL Mock: Processing LINKSTATS command: %s=%s\n", field, value);
        
        if (strcmp(field, "packets_sent") == 0) {
            g_link_stats.packets_sent = strtoull(value, NULL, 10);
        } else if (strcmp(field, "packets_received") == 0) {
            g_link_stats.packets_received = strtoull(value, NULL, 10);
        } else if (strcmp(field, "bytes_sent") == 0) {
            g_link_stats.bytes_sent = strtoull(value, NULL, 10);
        } else if (strcmp(field, "bytes_received") == 0) {
            g_link_stats.bytes_received = strtoull(value, NULL, 10);
        } else if (strcmp(field, "errors_sent") == 0) {
            g_link_stats.errors_sent = strtoull(value, NULL, 10);
        } else if (strcmp(field, "errors_received") == 0) {
            g_link_stats.errors_received = strtoull(value, NULL, 10);
        } else if (strcmp(field, "fec_corrected") == 0) {
            g_link_stats.fec_corrected = strtoull(value, NULL, 10);
        } else if (strcmp(field, "fec_uncorrectable") == 0) {
            g_link_stats.fec_uncorrectable = strtoull(value, NULL, 10);
        }
    }
    else if (strcmp(type, "TRANSCVRSTATS") == 0) {
        char *field = strtok_r(NULL, ":", &saveptr);
        char *value = strtok_r(NULL, ":", &saveptr);
        if (!field || !value) return;
        
        printf("EPON HAL Mock: Processing TRANSCVRSTATS command: %s=%s\n", field, value);
        
        if (strcmp(field, "tx_power") == 0) {
            g_transceiver_stats.transmit_optical_level = atof(value);
        } else if (strcmp(field, "rx_power") == 0) {
            g_transceiver_stats.optical_signal_level = atof(value);
        } else if (strcmp(field, "temperature") == 0) {
            g_transceiver_stats.temperature = atof(value);
        } else if (strcmp(field, "bias_current") == 0) {
            g_transceiver_stats.bias_current = atof(value);
        } else if (strcmp(field, "voltage") == 0) {
            g_transceiver_stats.supply_voltage = atof(value);
        }
    }
    else if (strcmp(type, "INCREMENT") == 0) {
        char *field = strtok_r(NULL, ":", &saveptr);
        char *value = strtok_r(NULL, ":", &saveptr);
        if (!field || !value) return;
        
        uint64_t inc = strtoull(value, NULL, 10);
        printf("EPON HAL Mock: Processing INCREMENT command: %s+=%llu\n", field, (unsigned long long)inc);
        
        if (strcmp(field, "packets_sent") == 0) {
            g_link_stats.packets_sent += inc;
        } else if (strcmp(field, "packets_received") == 0) {
            g_link_stats.packets_received += inc;
        } else if (strcmp(field, "bytes_sent") == 0) {
            g_link_stats.bytes_sent += inc;
        } else if (strcmp(field, "bytes_received") == 0) {
            g_link_stats.bytes_received += inc;
        } else if (strcmp(field, "errors_sent") == 0) {
            g_link_stats.errors_sent += inc;
        } else if (strcmp(field, "errors_received") == 0) {
            g_link_stats.errors_received += inc;
        }
    }
    else if (strcmp(type, "INVALIDATE") == 0) {
        printf("EPON HAL Mock: Cache invalidation requested - triggering ONU status events\n");
        
        /* Invalidate EPON Manager cache by triggering status change events */
        if (g_initialized && g_config.status_callback) {
            /* Trigger DOWN then UP to invalidate cache without changing actual state */
            g_config.status_callback(EPON_ONU_STATUS_DEREGISTRATION);
            usleep(10000); /* 10ms delay between events */
            g_config.status_callback(EPON_ONU_STATUS_REGISTRATION);
            printf("EPON HAL Mock: Cache invalidation complete (sent DEREGISTRATION -> REGISTRATION)\n");
        } else {
            printf("EPON HAL Mock: Warning - cache invalidation failed (callbacks not registered)\n");
        }
    }
}

static void* control_thread_func(void *arg) {
    (void)arg;
    struct sockaddr_un addr;
    int server_fd, client_fd;
    char buffer[MAX_COMMAND_LEN];
    
    /* Create socket */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("EPON HAL Mock: Failed to create control socket: %s\n", strerror(errno));
        return NULL;
    }
    
    /* Remove old socket file if exists */
    unlink(EPON_HAL_MOCK_SOCKET_PATH);
    
    /* Bind socket */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, EPON_HAL_MOCK_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("EPON HAL Mock: Failed to bind control socket: %s\n", strerror(errno));
        close(server_fd);
        return NULL;
    }
    
    /* Listen for connections */
    if (listen(server_fd, 5) < 0) {
        printf("EPON HAL Mock: Failed to listen on control socket: %s\n", strerror(errno));
        close(server_fd);
        unlink(EPON_HAL_MOCK_SOCKET_PATH);
        return NULL;
    }
    
    printf("EPON HAL Mock: Control socket listening on %s\n", EPON_HAL_MOCK_SOCKET_PATH);
    g_control_socket = server_fd;
    
    /* Accept and process commands */
    while (g_control_running) {
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int ret = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue; /* Timeout */
        
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            break;
        }
        
        /* Read command */
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            process_command(buffer);
            
            /* Send acknowledgment */
            const char *ack = "OK\n";
            write(client_fd, ack, strlen(ack));
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    unlink(EPON_HAL_MOCK_SOCKET_PATH);
    printf("EPON HAL Mock: Control socket closed\n");
    return NULL;
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
    
    /* Start control socket thread for IPC */
    g_control_running = true;
    if (pthread_create(&g_control_thread, NULL, control_thread_func, NULL) != 0) {
        printf("EPON HAL Mock: Warning - failed to create control thread\n");
        /* Non-fatal - continue without control socket */
    }
    
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
    llid_list->llid_count = g_llid_count;
    
    if (llid_list->llid_count > 0) {
        llid_list->llid_list = (epon_llid_info_t *)malloc(llid_list->llid_count * sizeof(epon_llid_info_t));
        if (!llid_list->llid_list) {
            return EPON_HAL_ERROR_MEMORY;
        }
        
        /* Copy all LLID entries */
        memcpy(llid_list->llid_list, g_llid_entries, llid_list->llid_count * sizeof(epon_llid_info_t));
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
    
    /* Populate CPE table */
    cpe_table->max_cpe = 32;
    cpe_table->static_cpe_count = g_static_cpe_count;
    cpe_table->dynamic_cpe_count = g_dynamic_cpe_count;
    
    /* Allocate and copy CPE entries */
    uint32_t total_cpe = g_static_cpe_count + g_dynamic_cpe_count;
    if (total_cpe > 0) {
        cpe_table->cpe_list = (dpoe_cpe_mac_entry_t *)malloc(total_cpe * sizeof(dpoe_cpe_mac_entry_t));
        if (!cpe_table->cpe_list) {
            return EPON_HAL_ERROR_MEMORY;
        }
        memcpy(cpe_table->cpe_list, g_cpe_entries, total_cpe * sizeof(dpoe_cpe_mac_entry_t));
    } else {
        cpe_table->cpe_list = NULL;
    }
    
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

void epon_hal_mock_trigger_alarm(epon_alarm_type_t type, uint32_t alarm_value, uint16_t llid, bool is_active) {
    if (g_initialized && g_config.alarm_callback) {
        epon_alarm_info_t alarm_info;
        alarm_info.alarm_type = type;
        if (type == EPON_ALARM_TYPE_STANDARD) {
            alarm_info.standard_alarm = (epon_hal_alarm_t)alarm_value;
        } else {
            alarm_info.vendor_alarm = (epon_vendor_alarm_t)alarm_value;
        }
        alarm_info.llid = llid;
        alarm_info.is_active = is_active;
        
        printf("EPON HAL Mock: Triggering alarm callback - type=%d, alarm=%u, llid=%u, active=%d\n", 
               type, alarm_value, llid, is_active);
        g_config.alarm_callback(&alarm_info);
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
