/**
 * @file test_event_processing.c
 * @brief Test Phase 5 event processing
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../include/epon_hal.h"
#include "../../tests/hal_mock/epon_hal_mock.h"

int main(void) {
    printf("=== Phase 5 Event Processing Test ===\n\n");
    
    // Initialize HAL mock
    printf("Step 1: Initializing HAL mock...\n");
    epon_hal_config_t config;
    config.struct_size = sizeof(config);
    config.status_callback = NULL; // Will be registered by controller
    config.alarm_callback = NULL;
    config.interface_status_callback = NULL;
    
    if (epon_hal_init(&config) != EPON_HAL_SUCCESS) {
        printf("ERROR: Failed to initialize HAL\n");
        return 1;
    }
    printf("HAL initialized\n\n");
    
    // Wait a bit for controller to start (if running)
    sleep(2);
    
    // Trigger ONU status event
    printf("Step 2: Triggering ONU REGISTRATION event...\n");
    epon_hal_mock_trigger_status(EPON_ONU_STATUS_REGISTRATION);
    sleep(1);
    
    // Trigger interface status event
    printf("\nStep 3: Triggering Interface UP event...\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_UP);
    sleep(1);
    
    // Trigger alarm event
    printf("\nStep 4: Triggering ALARM event (LOS)...\n");
    epon_hal_mock_trigger_alarm(EPON_HAL_ALARM_LOS, true);
    sleep(1);
    
    // Trigger interface down event
    printf("\nStep 5: Triggering Interface DOWN event...\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_DOWN);
    sleep(1);
    
    // Trigger ONU deregistration
    printf("\nStep 6: Triggering ONU DEREGISTRATION event...\n");
    epon_hal_mock_trigger_status(EPON_ONU_STATUS_DEREGISTRATION);
    sleep(1);
    
    printf("\n=== Test Complete ===\n");
    printf("Check EPON Manager logs to verify event processing\n");
    
    return 0;
}
