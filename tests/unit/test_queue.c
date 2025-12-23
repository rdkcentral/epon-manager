/**
 * @file test_queue.c
 * @brief Test event queue
 */

#include "../../src/core/data_structures/eponMgr_queue.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("=== EPON Queue Test ===\n\n");
    
    /* Test 1: Initialize queue */
    printf("Test 1: Initialize queue with capacity 10\n");
    eponMgr_queue_t queue;
    if (eponMgr_queue_init(&queue, 10) == 0) {
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - init failed\n\n");
        return 1;
    }
    
    /* Test 2: Check empty queue */
    printf("Test 2: Check if new queue is empty\n");
    if (eponMgr_queue_is_empty(&queue) && eponMgr_queue_size(&queue) == 0) {
        printf("  ✓ PASS - queue is empty\n\n");
    } else {
        printf("  ✗ FAIL - queue should be empty\n\n");
        return 1;
    }
    
    /* Test 3: Push event */
    printf("Test 3: Push ONU status event\n");
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
    event.data.onu_status.status = EPON_ONU_STATUS_REGISTRATION;
    
    if (eponMgr_queue_push(&queue, &event) == 0 && eponMgr_queue_size(&queue) == 1) {
        printf("  ✓ PASS - event pushed, size=1\n\n");
    } else {
        printf("  ✗ FAIL - push failed\n\n");
        return 1;
    }
    
    /* Test 4: Pop event */
    printf("Test 4: Pop event from queue\n");
    eponMgr_event_t popped;
    if (eponMgr_queue_pop(&queue, &popped) == 0 &&
        popped.type == EPONMGR_EVENT_TYPE_ONU_STATUS &&
        popped.data.onu_status.status == EPON_ONU_STATUS_REGISTRATION &&
        eponMgr_queue_is_empty(&queue)) {
        printf("  ✓ PASS - correct event popped, queue empty\n\n");
    } else {
        printf("  ✗ FAIL - pop failed or wrong data\n\n");
        return 1;
    }
    
    /* Test 5: Push multiple events */
    printf("Test 5: Push 5 different event types\n");
    
    /* ONU status */
    event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
    event.data.onu_status.status = EPON_ONU_STATUS_REGISTRATION;
    eponMgr_queue_push(&queue, &event);
    
    /* Interface status */
    event.type = EPONMGR_EVENT_TYPE_INTERFACE_STATUS;
    strncpy(event.data.interface_status.info.name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    event.data.interface_status.info.status = EPON_ONU_INTF_STATUS_LINK_UP;
    eponMgr_queue_push(&queue, &event);
    
    /* Alarm */
    event.type = EPONMGR_EVENT_TYPE_ALARM;
    event.data.alarm.alarm = EPON_HAL_ALARM_LOS;
    event.data.alarm.is_active = true;
    eponMgr_queue_push(&queue, &event);
    
    /* Another interface */
    event.type = EPONMGR_EVENT_TYPE_INTERFACE_STATUS;
    strncpy(event.data.interface_status.info.name, "veip1", EPON_HAL_INTERFACE_NAME_LEN - 1);
    event.data.interface_status.info.status = EPON_ONU_INTF_STATUS_LINK_DOWN;
    eponMgr_queue_push(&queue, &event);
    
    /* Another alarm */
    event.type = EPONMGR_EVENT_TYPE_ALARM;
    event.data.alarm.alarm = EPON_HAL_ALARM_LOS;
    event.data.alarm.is_active = false;
    eponMgr_queue_push(&queue, &event);
    
    if (eponMgr_queue_size(&queue) == 5) {
        printf("  ✓ PASS - 5 events in queue\n\n");
    } else {
        printf("  ✗ FAIL - wrong size: %u\n\n", eponMgr_queue_size(&queue));
        return 1;
    }
    
    /* Test 6: Pop all events in order */
    printf("Test 6: Pop all events and verify FIFO order\n");
    
    /* Event 1: ONU status */
    eponMgr_queue_pop(&queue, &popped);
    if (popped.type != EPONMGR_EVENT_TYPE_ONU_STATUS) {
        printf("  ✗ FAIL - Event 1 wrong type\n\n");
        return 1;
    }
    
    /* Event 2: Interface veip0 UP */
    eponMgr_queue_pop(&queue, &popped);
    if (popped.type != EPONMGR_EVENT_TYPE_INTERFACE_STATUS ||
        strcmp(popped.data.interface_status.info.name, "veip0") != 0) {
        printf("  ✗ FAIL - Event 2 wrong\n\n");
        return 1;
    }
    
    /* Event 3: Alarm LOS active */
    eponMgr_queue_pop(&queue, &popped);
    if (popped.type != EPONMGR_EVENT_TYPE_ALARM ||
        popped.data.alarm.is_active != true) {
        printf("  ✗ FAIL - Event 3 wrong\n\n");
        return 1;
    }
    
    /* Event 4: Interface veip1 DOWN */
    eponMgr_queue_pop(&queue, &popped);
    if (popped.type != EPONMGR_EVENT_TYPE_INTERFACE_STATUS ||
        strcmp(popped.data.interface_status.info.name, "veip1") != 0) {
        printf("  ✗ FAIL - Event 4 wrong\n\n");
        return 1;
    }
    
    /* Event 5: Alarm LOS cleared */
    eponMgr_queue_pop(&queue, &popped);
    if (popped.type != EPONMGR_EVENT_TYPE_ALARM ||
        popped.data.alarm.is_active != false) {
        printf("  ✗ FAIL - Event 5 wrong\n\n");
        return 1;
    }
    
    if (eponMgr_queue_is_empty(&queue)) {
        printf("  ✓ PASS - all events popped in correct FIFO order\n\n");
    } else {
        printf("  ✗ FAIL - queue not empty\n\n");
        return 1;
    }
    
    /* Test 7: Fill queue to capacity */
    printf("Test 7: Fill queue to capacity (10 events)\n");
    for (int i = 0; i < 10; i++) {
        event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
        event.data.onu_status.status = EPON_ONU_STATUS_REGISTRATION;
        eponMgr_queue_push(&queue, &event);
    }
    
    if (eponMgr_queue_is_full(&queue) && eponMgr_queue_size(&queue) == 10) {
        printf("  ✓ PASS - queue full\n\n");
    } else {
        printf("  ✗ FAIL - queue should be full\n\n");
        return 1;
    }
    
    /* Test 8: Try to push when full */
    printf("Test 8: Try to push when full (should fail)\n");
    if (eponMgr_queue_push(&queue, &event) != 0) {
        printf("  ✓ PASS - push correctly rejected\n\n");
    } else {
        printf("  ✗ FAIL - push should have failed\n\n");
        return 1;
    }
    
    /* Test 9: Destroy queue */
    printf("Test 9: Destroy queue\n");
    eponMgr_queue_destroy(&queue);
    printf("  ✓ PASS - queue destroyed\n\n");
    
    printf("=== All Queue Tests Passed ===\n");
    return 0;
}
