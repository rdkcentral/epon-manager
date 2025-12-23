/**
 * @file test_hal_init.c
 * @brief Test HAL initialization and version
 */

#include "../hal_mock/epon_hal_mock.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("=== EPON HAL Init Test ===\n\n");
    
    /* Test 1: Get version */
    printf("Test 1: Get HAL version\n");
    uint32_t version = epon_hal_get_version();
    printf("  Version: 0x%08X (Major=%d, Minor=%d, Patch=%d)\n",
           version, version >> 24, (version >> 16) & 0xFF, version & 0xFFFF);
    printf("  Expected: 0x%08X\n", EPON_HAL_API_VERSION);
    if (version == EPON_HAL_API_VERSION) {
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL\n\n");
        return 1;
    }
    
    /* Test 2: Init with NULL config */
    printf("Test 2: Init with NULL config (should fail)\n");
    int ret = epon_hal_init(NULL);
    if (ret == EPON_HAL_ERROR_INVALID_PARAM) {
        printf("  ✓ PASS - returned INVALID_PARAM\n\n");
    } else {
        printf("  ✗ FAIL - expected INVALID_PARAM, got %d\n\n", ret);
        return 1;
    }
    
    /* Test 3: Init with invalid struct_size */
    printf("Test 3: Init with invalid struct_size (should fail)\n");
    epon_hal_config_t config = {0};
    config.struct_size = 0; /* Invalid */
    ret = epon_hal_init(&config);
    if (ret == EPON_HAL_ERROR_INVALID_PARAM) {
        printf("  ✓ PASS - returned INVALID_PARAM\n\n");
    } else {
        printf("  ✗ FAIL - expected INVALID_PARAM, got %d\n\n", ret);
        return 1;
    }
    
    /* Test 4: Valid initialization */
    printf("Test 4: Valid initialization\n");
    config.struct_size = sizeof(epon_hal_config_t);
    config.dpoe_supported = false;
    config.status_callback = NULL;
    config.alarm_callback = NULL;
    config.interface_status_callback = NULL;
    
    ret = epon_hal_init(&config);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - expected SUCCESS, got %d\n\n", ret);
        return 1;
    }
    
    /* Test 5: Call API after init (should work) */
    printf("Test 5: Get manufacturer info after init\n");
    epon_onu_manufacturer_info_t mfg_info = {0};
    mfg_info.struct_size = sizeof(mfg_info);
    ret = epon_hal_get_manufacturer_info(&mfg_info);
    if (ret == EPON_HAL_SUCCESS) {
        printf("  Manufacturer: %s\n", mfg_info.manufacturer);
        printf("  Model: %s\n", mfg_info.model_number);
        printf("  HW Version: %s\n", mfg_info.hardware_version);
        printf("  SW Version: %s\n", mfg_info.software_version);
        printf("  Serial: %s\n", mfg_info.serial_number);
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - got error %d\n\n", ret);
        return 1;
    }
    
    printf("=== All Init Tests Passed ===\n");
    return 0;
}
