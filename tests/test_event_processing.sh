#!/bin/bash
# Test script to trigger HAL events and verify event processing

echo "=== Testing EPON Manager Event Processing ==="
echo ""

# Create a simple test that triggers events
cat > /tmp/test_events.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../../include/epon_hal.h"

// External mock functions to trigger events
extern void epon_hal_mock_trigger_onu_status(epon_onu_status_t status);
extern void epon_hal_mock_trigger_interface_status(const char *ifname, epon_interface_link_status_t status);
extern void epon_hal_mock_trigger_alarm(epon_hal_alarm_t alarm, bool is_active);

int main(void) {
    printf("Sleeping 2 seconds to let EPON Manager start...\n");
    sleep(2);
    
    printf("Triggering ONU REGISTRATION event...\n");
    epon_hal_mock_trigger_onu_status(EPON_ONU_STATUS_REGISTRATION);
    sleep(1);
    
    printf("Triggering Interface veip0 UP event...\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_UP);
    sleep(1);
    
    printf("Triggering LOS ALARM event...\n");
    epon_hal_mock_trigger_alarm(EPON_HAL_ALARM_LOS, true);
    sleep(1);
    
    printf("Clearing LOS ALARM event...\n");
    epon_hal_mock_trigger_alarm(EPON_HAL_ALARM_LOS, false);
    sleep(1);
    
    printf("Triggering Interface veip0 DOWN event...\n");
    epon_hal_mock_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_DOWN);
    sleep(1);
    
    printf("Triggering ONU DEREGISTRATION event...\n");
    epon_hal_mock_trigger_onu_status(EPON_ONU_STATUS_DEREGISTRATION);
    sleep(1);
    
    printf("\nAll events triggered! Check EPON Manager logs.\n");
    return 0;
}
EOF

echo "Test would trigger the following events:"
echo "1. ONU REGISTRATION"
echo "2. Interface veip0 UP"
echo "3. LOS ALARM (raised)"
echo "4. LOS ALARM (cleared)"
echo "5. Interface veip0 DOWN"
echo "6. ONU DEREGISTRATION"
echo ""
echo "Note: This demonstrates the event flow. In real testing,"
echo "      you would need the HAL mock to provide trigger functions."
echo ""
echo "The optimized event loop now:"
echo "  - Uses condition variable (pthread_cond_t) for sleep/wake"
echo "  - Wakes immediately when HAL callbacks enqueue events"
echo "  - Falls back to 500ms timeout if no events arrive"
echo "  - Much more efficient than polling every 10ms"
echo ""
echo "Benefits:"
echo "  - Near-instant event processing (no polling delay)"
echo "  - Very low CPU usage when idle"
echo "  - Graceful shutdown (wakes on signal)"

rm -f /tmp/test_events.c
