/**
 * @file test_hal_stats.c
 * @brief Test HAL statistics APIs
 */

#include "../hal_mock/epon_hal_mock.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    printf("=== EPON HAL Statistics Test ===\n\n");
    
    /* Initialize HAL */
    epon_hal_config_t config = {0};
    config.struct_size = sizeof(config);
    config.dpoe_supported = false;
    
    int ret = epon_hal_init(&config);
    if (ret != EPON_HAL_SUCCESS) {
        printf("Init failed: %d\n", ret);
        return 1;
    }
    
    /* Test 1: Get link stats */
    printf("Test 1: Get link statistics\n");
    epon_hal_link_stats_t link_stats = {0};
    link_stats.struct_size = sizeof(link_stats);
    ret = epon_hal_get_link_stats(&link_stats);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Packets sent: %lu\n", link_stats.packets_sent);
        printf("  Packets received: %lu\n", link_stats.packets_received);
        printf("  Bytes sent: %lu\n", link_stats.bytes_sent);
        printf("  Bytes received: %lu\n", link_stats.bytes_received);
        printf("  Max bit rate: %u Mbps\n", link_stats.max_bit_rate);
        printf("  FEC corrected: %lu\n", link_stats.fec_corrected);
        printf("  FEC uncorrectable: %lu\n", link_stats.fec_uncorrectable);
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    /* Test 2: Get transceiver stats */
    printf("Test 2: Get transceiver statistics\n");
    epon_hal_transceiver_stats_t xcvr_stats = {0};
    xcvr_stats.struct_size = sizeof(xcvr_stats);
    ret = epon_hal_get_transceiver_stats(&xcvr_stats);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  TX optical level: %.2f dBm\n", xcvr_stats.transmit_optical_level);
        printf("  RX optical level: %.2f dBm\n", xcvr_stats.optical_signal_level);
        printf("  Bias current: %.2f mA\n", xcvr_stats.bias_current);
        printf("  Temperature: %.2f °C\n", xcvr_stats.temperature);
        printf("  Supply voltage: %.2f V\n", xcvr_stats.supply_voltage);
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    /* Test 3: Clear stats */
    printf("Test 3: Clear statistics\n");
    ret = epon_hal_clear_stats();
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Stats cleared\n");
        
        /* Verify stats are cleared */
        memset(&link_stats, 0, sizeof(link_stats));
        link_stats.struct_size = sizeof(link_stats);
        ret = epon_hal_get_link_stats(&link_stats);
        if (ret == EPON_HAL_SUCCESS && 
            link_stats.packets_sent == 0 && 
            link_stats.packets_received == 0) {
            printf("  ✓ PASS - stats reset to 0\n\n");
        } else {
            printf("  ✗ FAIL - stats not cleared\n\n");
            return 1;
        }
    } else {
        printf("  ✗ FAIL - clear_stats failed: %d\n\n", ret);
        return 1;
    }
    
    /* Test 4: Get LLID info */
    printf("Test 4: Get LLID information\n");
    epon_llid_list_t llid_list = {0};
    ret = epon_hal_get_llid_info(&llid_list);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Max LLID count: %u\n", llid_list.max_llid_count);
        printf("  Active LLID count: %u\n", llid_list.llid_count);
        
        for (uint32_t i = 0; i < llid_list.llid_count; i++) {
            printf("  LLID[%u]: value=%u, mode=%d, state=%d\n",
                   i, llid_list.llid_list[i].llid_value,
                   llid_list.llid_list[i].mode,
                   llid_list.llid_list[i].state);
        }
        
        /* Free allocated memory */
        if (llid_list.llid_list) {
            free(llid_list.llid_list);
        }
        
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    /* Test 5: Get link info */
    printf("Test 5: Get link information\n");
    epon_hal_link_info_t link_info = {0};
    ret = epon_hal_get_link_info(&link_info);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Mode: %s\n", link_info.mode);
        printf("  Encryption: %d\n", link_info.encryption);
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    /* Test 6: Get interface list */
    printf("Test 6: Get interface list\n");
    epon_interface_list_t if_list = {0};
    ret = epon_hal_get_interface_list(&if_list);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Interface count: %u\n", if_list.interface_count);
        for (uint32_t i = 0; i < if_list.interface_count; i++) {
            printf("  Interface[%u]: %s, status=%d\n",
                   i, if_list.interface[i].name, if_list.interface[i].status);
        }
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    /* Test 7: Get OLT info */
    printf("Test 7: Get OLT information\n");
    epon_olt_info_t olt_info = {0};
    olt_info.struct_size = sizeof(olt_info);
    ret = epon_hal_get_olt_info(&olt_info);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  OLT MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               olt_info.mac_address[0], olt_info.mac_address[1],
               olt_info.mac_address[2], olt_info.mac_address[3],
               olt_info.mac_address[4], olt_info.mac_address[5]);
        printf("  OLT OUI: %02x:%02x:%02x\n",
               olt_info.vendor_oui[0], olt_info.vendor_oui[1], olt_info.vendor_oui[2]);
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    printf("=== All Statistics Tests Passed ===\n");
    return 0;
}
