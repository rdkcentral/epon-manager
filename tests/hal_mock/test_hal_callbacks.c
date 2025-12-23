/**
 * @file test_hal_callbacks.c
 * @brief Test HAL callback mechanisms
 */

#include "../hal_mock/epon_hal_mock.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Callback tracking */
static int g_status_callback_count = 0;
static epon_onu_status_t g_last_status;

static int g_alarm_callback_count = 0;
static epon_hal_alarm_t g_last_alarm;
static bool g_last_alarm_active;

static int g_interface_callback_count = 0;
static char g_last_interface_name[EPON_HAL_INTERFACE_NAME_LEN];
static epon_interface_link_status_t g_last_interface_status;

/* Callback implementations */
void status_callback(epon_onu_status_t status) {
    g_status_callback_count++;
    g_last_status = status;
    printf("  → Status callback invoked: status=%d\n", status);
}

void alarm_callback(epon_hal_alarm_t alarm, bool is_active) {
    g_alarm_callback_count++;
    g_last_alarm = alarm;
    g_last_alarm_active = is_active;
    printf("  → Alarm callback invoked: alarm=%d, active=%d\n", alarm, is_active);
}

void interface_status_callback(epon_onu_interface_info_t status) {
    g_interface_callback_count++;
    strncpy(g_last_interface_name, status.name, EPON_HAL_INTERFACE_NAME_LEN - 1);
    g_last_interface_status = status.status;
    printf("  → Interface callback invoked: interface=%s, status=%d\n", 
           status.name, status.status);
}

int main(void) {
    printf("=== EPON HAL Callback Test ===\n\n");
    
    /* Initialize HAL with callbacks */
    printf("Test 1: Initialize HAL with callbacks\n");
    epon_hal_config_t config = {0};
    config.struct_size = sizeof(config);
    config.dpoe_supported = false;
    config.status_callback = status_callback;
    config.alarm_callback = alarm_callback;
    config.interface_status_callback = interface_status_callback;
    
    int ret = epon_hal_init(&config);
    if (ret != EPON_HAL_SUCCESS) {
        printf("  ✗ FAIL - init failed: %d\n", ret);
        return 1;
    }
    printf("  ✓ PASS\n\n");
    
    /* Test 2: Trigger status callback */
    printf("Test 2: Trigger ONU status callback\n");
    epon_hal_mock_trigger_status(EPON_ONU_STATUS_REGISTRATION);
    if (g_status_callback_count == 1 && g_last_status == EPON_ONU_STATUS_REGISTRATION) {
        printf("  ✓ PASS - callback invoked correctly\n\n");
    } else {
        printf("  ✗ FAIL - callback not invoked or wrong status\n\n");
        return 1;
    }
    
    /* Test 3: Trigger alarm callback */
    printf("Test 3: Trigger alarm callback (LOS alarm raised)\n");
    epon_hal_mock_trigger_alarm(EPON_HAL_ALARM_LOS, true);
    if (g_alarm_callback_count == 1 && 
        g_last_alarm == EPON_HAL_ALARM_LOS && 
        g_last_alarm_active == true) {
        printf("  ✓ PASS - alarm callback invoked correctly\n\n");
    } else {
        printf("  ✗ FAIL - alarm callback not invoked or wrong data\n\n");
        return 1;
    }
    
    /* Test 4: Trigger alarm cleared */
    printf("Test 4: Trigger alarm callback (LOS alarm cleared)\n");
    epon_hal_mock_trigger_alarm(EPON_HAL_ALARM_LOS, false);
    if (g_alarm_callback_count == 2 && 
        g_last_alarm == EPON_HAL_ALARM_LOS && 
        g_last_alarm_active == false) {
        printf("  ✓ PASS - alarm cleared callback invoked correctly\n\n");
    } else {
        printf("  ✗ FAIL - alarm cleared callback not invoked\n\n");
        return 1;
    }
    
    /* Test 5: Trigger interface status callback */
    printf("Test 5: Trigger interface status callback (veip0 UP)\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_UP);
    if (g_interface_callback_count == 1 && 
        strcmp(g_last_interface_name, "veip0") == 0 &&
        g_last_interface_status == EPON_ONU_INTF_STATUS_LINK_UP) {
        printf("  ✓ PASS - interface callback invoked correctly\n\n");
    } else {
        printf("  ✗ FAIL - interface callback not invoked or wrong data\n\n");
        return 1;
    }
    
    /* Test 6: Multiple interface status changes */
    printf("Test 6: Multiple interface events\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_DOWN);
    epon_hal_mock_trigger_interface_status("veip1", EPON_ONU_INTF_STATUS_LINK_UP);
    if (g_interface_callback_count == 3) {
        printf("  ✓ PASS - all callbacks invoked\n\n");
    } else {
        printf("  ✗ FAIL - expected 3 callbacks, got %d\n\n", g_interface_callback_count);
        return 1;
    }
    
    printf("=== All Callback Tests Passed ===\n");
    return 0;
}
