/*
 * Unit Tests for EPON Manager Data Structures
 * Tests: Interface List, LLID List, CPE List, ONU State
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "eponMgr_interface_list.h"
#include "eponMgr_llid_list.h"
#include "eponMgr_cpe_list.h"
#include "eponMgr_onu_state.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("Running test: %s\n", #name); \
    tests_run++; \
    if (name()) { \
        printf("  ✓ PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("  ✗ FAILED\n"); \
    } \
} while(0)

/* ========================================================================
 * Interface List Tests
 * ======================================================================== */

int test_interface_list_init_destroy()
{
    eponMgr_interface_list_t list;
    
    int ret = eponMgr_interface_list_init(&list);
    if (ret != 0) return 0;
    
    if (eponMgr_interface_list_count(&list) != 0) return 0;
    
    eponMgr_interface_list_destroy(&list);
    return 1;
}

int test_interface_list_add_update()
{
    eponMgr_interface_list_t list;
    epon_onu_interface_info_t intf_info;
    
    eponMgr_interface_list_init(&list);
    
    // Add veip0 with LINK_DOWN
    memset(&intf_info, 0, sizeof(intf_info));
    strncpy(intf_info.name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_DOWN;
    
    int ret = eponMgr_interface_list_update(&list, &intf_info);
    if (ret != 0) return 0;
    if (eponMgr_interface_list_count(&list) != 1) return 0;
    
    // Update veip0 to LINK_UP
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_UP;
    ret = eponMgr_interface_list_update(&list, &intf_info);
    if (ret != 0) return 0;
    if (eponMgr_interface_list_count(&list) != 1) return 0;
    
    // Add veip1 with LINK_UP
    strncpy(intf_info.name, "veip1", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_UP;
    ret = eponMgr_interface_list_update(&list, &intf_info);
    if (ret != 0) return 0;
    if (eponMgr_interface_list_count(&list) != 2) return 0;
    
    eponMgr_interface_list_destroy(&list);
    return 1;
}

int test_interface_list_any_all()
{
    eponMgr_interface_list_t list;
    epon_onu_interface_info_t intf_info;
    
    eponMgr_interface_list_init(&list);
    
    // Add one DOWN interface
    memset(&intf_info, 0, sizeof(intf_info));
    strncpy(intf_info.name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_DOWN;
    eponMgr_interface_list_update(&list, &intf_info);
    
    // Verify all down
    epon_onu_interface_info_t retrieved;
    if (eponMgr_interface_list_get(&list, "veip0", &retrieved) != 0) return 0;
    if (retrieved.status != EPON_ONU_INTF_STATUS_LINK_DOWN) return 0;
    
    // Bring it UP
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_UP;
    eponMgr_interface_list_update(&list, &intf_info);
    if (eponMgr_interface_list_get(&list, "veip0", &retrieved) != 0) return 0;
    if (retrieved.status != EPON_ONU_INTF_STATUS_LINK_UP) return 0;
    
    // Add second interface DOWN
    strncpy(intf_info.name, "veip1", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_DOWN;
    eponMgr_interface_list_update(&list, &intf_info);
    
    if (eponMgr_interface_list_count(&list) != 2) return 0;
    
    eponMgr_interface_list_destroy(&list);
    return 1;
}

int test_interface_list_to_hal()
{
    eponMgr_interface_list_t list;
    epon_onu_interface_info_t intf_info;
    
    eponMgr_interface_list_init(&list);
    
    memset(&intf_info, 0, sizeof(intf_info));
    strncpy(intf_info.name, "veip0", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_UP;
    eponMgr_interface_list_update(&list, &intf_info);
    
    strncpy(intf_info.name, "veip1", EPON_HAL_INTERFACE_NAME_LEN - 1);
    intf_info.status = EPON_ONU_INTF_STATUS_LINK_DOWN;
    eponMgr_interface_list_update(&list, &intf_info);
    
    // Verify count - caller can iterate using get_at()
    if (eponMgr_interface_list_count(&list) != 2) return 0;
    
    eponMgr_interface_list_destroy(&list);
    return 1;
}

/* ========================================================================
 * LLID List Tests
 * ======================================================================== */

int test_llid_list_init_destroy()
{
    eponMgr_llid_list_t list;
    
    int ret = eponMgr_llid_list_init(&list, 8);
    if (ret != 0) return 0;
    if (eponMgr_llid_list_count(&list) != 0) return 0;
    if (list.llid_list.max_llid_count != 8) return 0;
    
    eponMgr_llid_list_destroy(&list);
    return 1;
}

int test_llid_list_add_update()
{
    eponMgr_llid_list_t list;
    epon_llid_info_t llid_info;
    
    eponMgr_llid_list_init(&list, 8);
    
    // Add LLID
    memset(&llid_info, 0, sizeof(llid_info));
    llid_info.llid_value = 100;
    llid_info.mode = EPON_LLID_MODE_UNICAST;
    llid_info.state = EPON_LLID_STATE_REGISTERING;
    llid_info.forwarding_state = EPON_LLID_FORWARDING_DISABLED;
    llid_info.encryption_enabled = false;
    
    int ret = eponMgr_llid_list_update(&list, &llid_info);
    if (ret != 0) return 0;
    if (eponMgr_llid_list_count(&list) != 1) return 0;
    
    // Update same LLID to REGISTERED
    llid_info.state = EPON_LLID_STATE_REGISTERED;
    ret = eponMgr_llid_list_update(&list, &llid_info);
    if (ret != 0) return 0;
    if (eponMgr_llid_list_count(&list) != 1) return 0;
    
    // Check registered - get and verify state
    epon_llid_info_t retrieved;
    if (eponMgr_llid_list_get(&list, 100, &retrieved) != 0) return 0;
    if (retrieved.state != EPON_LLID_STATE_REGISTERED) return 0;
    
    eponMgr_llid_list_destroy(&list);
    return 1;
}

int test_llid_list_remove()
{
    eponMgr_llid_list_t list;
    epon_llid_info_t llid_info;
    
    eponMgr_llid_list_init(&list, 8);
    
    memset(&llid_info, 0, sizeof(llid_info));
    llid_info.llid_value = 100;
    llid_info.state = EPON_LLID_STATE_REGISTERED;
    eponMgr_llid_list_update(&list, &llid_info);
    
    llid_info.llid_value = 101;
    eponMgr_llid_list_update(&list, &llid_info);
    
    if (eponMgr_llid_list_count(&list) != 2) return 0;
    
    // Remove LLID 100
    int ret = eponMgr_llid_list_remove(&list, 100);
    if (ret != 0) return 0;
    if (eponMgr_llid_list_count(&list) != 1) return 0;
    
    // Try to remove non-existent LLID
    ret = eponMgr_llid_list_remove(&list, 999);
    if (ret == 0) return 0; // Should fail
    
    eponMgr_llid_list_destroy(&list);
    return 1;
}

int test_llid_list_to_hal()
{
    eponMgr_llid_list_t list;
    epon_llid_info_t llid_info;
    
    eponMgr_llid_list_init(&list, 8);
    
    memset(&llid_info, 0, sizeof(llid_info));
    llid_info.llid_value = 100;
    llid_info.state = EPON_LLID_STATE_REGISTERED;
    eponMgr_llid_list_update(&list, &llid_info);
    
    // Verify count - caller can iterate using get_at()
    if (eponMgr_llid_list_count(&list) != 1) return 0;
    
    // Verify we can get it back
    if (eponMgr_llid_list_get(&list, 100, &llid_info) != 0) return 0;
    if (llid_info.llid_value != 100) return 0;
    
    eponMgr_llid_list_destroy(&list);
    return 1;
}

/* ========================================================================
 * CPE List Tests
 * ======================================================================== */

int test_cpe_list_init_destroy()
{
    eponMgr_cpe_list_t list;
    
    int ret = eponMgr_cpe_list_init(&list, 128);
    if (ret != 0) return 0;
    if (eponMgr_cpe_list_count(&list) != 0) return 0;
    if (list.cpe_table.max_cpe != 128) return 0;
    
    eponMgr_cpe_list_destroy(&list);
    return 1;
}

int test_cpe_list_add_update()
{
    eponMgr_cpe_list_t list;
    dpoe_cpe_mac_entry_t cpe_entry;
    
    eponMgr_cpe_list_init(&list, 128);
    
    // Add static CPE
    memset(&cpe_entry, 0, sizeof(cpe_entry));
    cpe_entry.mac_address[0] = 0x00;
    cpe_entry.mac_address[1] = 0x11;
    cpe_entry.mac_address[5] = 0x01;
    cpe_entry.type = DPOE_CPE_MAC_STATIC;
    cpe_entry.age_time = 0;
    
    int ret = eponMgr_cpe_list_update(&list, &cpe_entry);
    if (ret != 0) return 0;
    if (eponMgr_cpe_list_count(&list) != 1) return 0;
    
    // Add dynamic CPE
    cpe_entry.mac_address[5] = 0x02;
    cpe_entry.type = DPOE_CPE_MAC_DYNAMIC;
    cpe_entry.age_time = 300;
    
    ret = eponMgr_cpe_list_update(&list, &cpe_entry);
    if (ret != 0) return 0;
    if (eponMgr_cpe_list_count(&list) != 2) return 0;
    
    eponMgr_cpe_list_destroy(&list);
    return 1;
}

int test_cpe_list_clear_dynamic()
{
    eponMgr_cpe_list_t list;
    dpoe_cpe_mac_entry_t cpe_entry;
    
    eponMgr_cpe_list_init(&list, 128);
    
    // Add static and dynamic
    memset(&cpe_entry, 0, sizeof(cpe_entry));
    cpe_entry.mac_address[5] = 0x01;
    cpe_entry.type = DPOE_CPE_MAC_STATIC;
    eponMgr_cpe_list_update(&list, &cpe_entry);
    
    cpe_entry.mac_address[5] = 0x02;
    cpe_entry.type = DPOE_CPE_MAC_DYNAMIC;
    eponMgr_cpe_list_update(&list, &cpe_entry);
    
    if (eponMgr_cpe_list_count(&list) != 2) return 0;
    
    // Clear dynamic only
    eponMgr_cpe_list_clear_dynamic(&list);
    
    if (eponMgr_cpe_list_count(&list) != 1) return 0;
    
    eponMgr_cpe_list_destroy(&list);
    return 1;
}

int test_cpe_list_to_hal()
{
    eponMgr_cpe_list_t list;
    dpoe_cpe_mac_entry_t cpe_entry;
    
    eponMgr_cpe_list_init(&list, 128);
    
    memset(&cpe_entry, 0, sizeof(cpe_entry));
    cpe_entry.mac_address[5] = 0x01;
    cpe_entry.type = DPOE_CPE_MAC_STATIC;
    eponMgr_cpe_list_update(&list, &cpe_entry);
    
    cpe_entry.mac_address[5] = 0x02;
    cpe_entry.type = DPOE_CPE_MAC_DYNAMIC;
    eponMgr_cpe_list_update(&list, &cpe_entry);
    
    // Verify count - caller can iterate using get_at() and count types
    if (eponMgr_cpe_list_count(&list) != 2) return 0;
    
    // Verify we can get entries back
    uint8_t mac[6] = {0, 0, 0, 0, 0, 0x01};
    if (eponMgr_cpe_list_get(&list, mac, &cpe_entry) != 0) return 0;
    if (cpe_entry.type != DPOE_CPE_MAC_STATIC) return 0;
    
    eponMgr_cpe_list_destroy(&list);
    return 1;
}

/* ========================================================================
 * ONU State Tests
 * ======================================================================== */

int test_onu_state_init_destroy()
{
    eponMgr_onu_state_t state;
    
    int ret = eponMgr_onu_state_init(&state, false);
    if (ret != 0) return 0;
    if (state.current_status != EPON_ONU_STATUS_LOS) return 0;
    if (state.hal_initialized != false) return 0;
    
    eponMgr_onu_state_destroy(&state);
    return 1;
}

int test_onu_state_update_status()
{
    eponMgr_onu_state_t state;
    epon_onu_status_t status;
    
    eponMgr_onu_state_init(&state, false);
    
    // Update to REGISTRATION
    int ret = eponMgr_onu_state_update_status(&state, EPON_ONU_STATUS_REGISTRATION);
    if (ret != 0) return 0;
    
    eponMgr_onu_state_get_status(&state, &status);
    if (status != EPON_ONU_STATUS_REGISTRATION) return 0;
    
    // Check previous status
    if (state.previous_status != EPON_ONU_STATUS_LOS) return 0;
    
    // Check has_changed
    if (!eponMgr_onu_state_has_changed(&state)) return 0;
    
    eponMgr_onu_state_destroy(&state);
    return 1;
}

int test_onu_state_info_validity()
{
    eponMgr_onu_state_t state;
    epon_onu_manufacturer_info_t mfr_info;
    
    eponMgr_onu_state_init(&state, false);
    
    // Should fail when not valid
    int ret = eponMgr_onu_state_get_manufacturer_info(&state, &mfr_info);
    if (ret == 0) return 0; // Should fail
    
    // Set manufacturer info
    memset(&mfr_info, 0, sizeof(mfr_info));
    mfr_info.struct_size = sizeof(mfr_info);
    strcpy(mfr_info.manufacturer, "TestVendor");
    
    ret = eponMgr_onu_state_update_manufacturer_info(&state, &mfr_info);
    if (ret != 0) return 0;
    
    // Now should succeed
    epon_onu_manufacturer_info_t retrieved;
    ret = eponMgr_onu_state_get_manufacturer_info(&state, &retrieved);
    if (ret != 0) return 0;
    if (strcmp(retrieved.manufacturer, "TestVendor") != 0) return 0;
    
    eponMgr_onu_state_destroy(&state);
    return 1;
}

int test_onu_state_invalidate_all()
{
    eponMgr_onu_state_t state;
    epon_onu_manufacturer_info_t mfr_info;
    epon_hal_link_info_t link_info;
    
    eponMgr_onu_state_init(&state, false);
    
    // Set some info
    memset(&mfr_info, 0, sizeof(mfr_info));
    mfr_info.struct_size = sizeof(mfr_info);
    eponMgr_onu_state_update_manufacturer_info(&state, &mfr_info);
    
    memset(&link_info, 0, sizeof(link_info));
    eponMgr_onu_state_update_link_info(&state, &link_info);
    
    // Invalidate all
    eponMgr_onu_state_invalidate_all(&state);
    
    // Should all be invalid now
    if (state.manufacturer_info_valid) return 0;
    if (state.link_info_valid) return 0;
    if (state.olt_info_valid) return 0;
    
    eponMgr_onu_state_destroy(&state);
    return 1;
}

int test_onu_state_hal_init()
{
    eponMgr_onu_state_t state;
    
    eponMgr_onu_state_init(&state, false);
    
    if (eponMgr_onu_state_is_hal_initialized(&state)) return 0;
    
    eponMgr_onu_state_set_hal_initialized(&state);
    
    if (!eponMgr_onu_state_is_hal_initialized(&state)) return 0;
    
    eponMgr_onu_state_destroy(&state);
    return 1;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main()
{
    printf("\n");
    printf("===============================================\n");
    printf("EPON Manager Data Structures Unit Tests\n");
    printf("===============================================\n\n");

    // Interface List Tests
    printf("--- Interface List Tests ---\n");
    TEST(test_interface_list_init_destroy);
    TEST(test_interface_list_add_update);
    TEST(test_interface_list_any_all);
    TEST(test_interface_list_to_hal);
    printf("\n");

    // LLID List Tests
    printf("--- LLID List Tests ---\n");
    TEST(test_llid_list_init_destroy);
    TEST(test_llid_list_add_update);
    TEST(test_llid_list_remove);
    TEST(test_llid_list_to_hal);
    printf("\n");

    // CPE List Tests
    printf("--- CPE List Tests ---\n");
    TEST(test_cpe_list_init_destroy);
    TEST(test_cpe_list_add_update);
    TEST(test_cpe_list_clear_dynamic);
    TEST(test_cpe_list_to_hal);
    printf("\n");

    // ONU State Tests
    printf("--- ONU State Tests ---\n");
    TEST(test_onu_state_init_destroy);
    TEST(test_onu_state_update_status);
    TEST(test_onu_state_info_validity);
    TEST(test_onu_state_invalidate_all);
    TEST(test_onu_state_hal_init);
    printf("\n");

    // Summary
    printf("===============================================\n");
    printf("Test Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("===============================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
