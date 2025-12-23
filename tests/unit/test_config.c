/**
 * @file test_config.c
 * @brief Test configuration management
 */

#include "../../src/core/config/eponMgr_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    printf("=== EPON Config Test ===\n\n");
    
    /* Test 1: Initialize with defaults */
    printf("Test 1: Initialize with defaults\n");
    eponMgr_config_t config;
    eponMgr_config_init_defaults(&config);
    
    if (config.cache_ttl_seconds == 30 &&
        strcmp(config.log_level, "INFO") == 0 &&
        strcmp(config.log_directory, "./logs") == 0 &&
        config.dpoe_enabled == false &&
        config.use_dummy_rbus == true &&
        config.use_dummy_telemetry == true &&
        config.event_queue_size == 100) {
        printf("  ✓ PASS - all defaults correct\n\n");
    } else {
        printf("  ✗ FAIL - incorrect defaults\n\n");
        return 1;
    }
    
    /* Test 2: Validate good config */
    printf("Test 2: Validate valid configuration\n");
    if (eponMgr_config_validate(&config) == 0) {
        printf("  ✓ PASS - validation succeeded\n\n");
    } else {
        printf("  ✗ FAIL - validation failed\n\n");
        return 1;
    }
    
    /* Test 3: Validate bad config (invalid TTL) */
    printf("Test 3: Validate invalid cache TTL (should fail)\n");
    config.cache_ttl_seconds = 500; /* Too high */
    if (eponMgr_config_validate(&config) != 0) {
        printf("  ✓ PASS - validation correctly rejected bad TTL\n\n");
    } else {
        printf("  ✗ FAIL - validation should have failed\n\n");
        return 1;
    }
    
    /* Test 4: Validate bad log level */
    printf("Test 4: Validate invalid log level (should fail)\n");
    eponMgr_config_init_defaults(&config);
    strncpy(config.log_level, "INVALID", sizeof(config.log_level) - 1);
    if (eponMgr_config_validate(&config) != 0) {
        printf("  ✓ PASS - validation correctly rejected bad log level\n\n");
    } else {
        printf("  ✗ FAIL - validation should have failed\n\n");
        return 1;
    }
    
    /* Test 5: Create INI file and load it */
    printf("Test 5: Load configuration from INI file\n");
    
    /* Create test INI file */
    FILE *ini_file = fopen("./test_config.ini", "w");
    if (ini_file) {
        fprintf(ini_file, "# EPON Manager Test Configuration\n");
        fprintf(ini_file, "[cache]\n");
        fprintf(ini_file, "cache_ttl_seconds = 60\n");
        fprintf(ini_file, "\n");
        fprintf(ini_file, "[logging]\n");
        fprintf(ini_file, "log_level = DEBUG\n");
        fprintf(ini_file, "log_directory = /var/log/epon\n");
        fprintf(ini_file, "\n");
        fprintf(ini_file, "[hal]\n");
        fprintf(ini_file, "dpoe_enabled = true\n");
        fprintf(ini_file, "\n");
        fprintf(ini_file, "[rbus]\n");
        fprintf(ini_file, "use_dummy_rbus = false\n");
        fprintf(ini_file, "\n");
        fprintf(ini_file, "[telemetry]\n");
        fprintf(ini_file, "use_dummy_telemetry = false\n");
        fprintf(ini_file, "\n");
        fprintf(ini_file, "[queue]\n");
        fprintf(ini_file, "event_queue_size = 200\n");
        fclose(ini_file);
    }
    
    /* Load INI file */
    eponMgr_config_init_defaults(&config);
    if (eponMgr_config_load_file(&config, "./test_config.ini") == 0) {
        if (config.cache_ttl_seconds == 60 &&
            strcmp(config.log_level, "DEBUG") == 0 &&
            strcmp(config.log_directory, "/var/log/epon") == 0 &&
            config.dpoe_enabled == true &&
            config.use_dummy_rbus == false &&
            config.use_dummy_telemetry == false &&
            config.event_queue_size == 200) {
            printf("  ✓ PASS - INI file loaded correctly\n\n");
        } else {
            printf("  ✗ FAIL - INI values incorrect\n\n");
            return 1;
        }
    } else {
        printf("  ✗ FAIL - INI file load failed\n\n");
        return 1;
    }
    
    /* Test 6: Environment variable override */
    printf("Test 6: Override with environment variables\n");
    setenv("EPON_CACHE_TTL", "90", 1);
    setenv("EPONMGR_LOG_LEVEL", "ERROR", 1);
    
    eponMgr_config_load_env(&config);
    
    if (config.cache_ttl_seconds == 90 &&
        strcmp(config.log_level, "ERROR") == 0) {
        printf("  ✓ PASS - environment override worked\n\n");
    } else {
        printf("  ✗ FAIL - environment override failed\n\n");
        return 1;
    }
    
    /* Test 7: Print config */
    printf("Test 7: Print configuration\n");
    eponMgr_config_init_defaults(&config);
    eponMgr_config_print(&config);
    printf("  ✓ PASS\n\n");
    
    /* Cleanup */
    remove("./test_config.ini");
    
    printf("=== All Config Tests Passed ===\n");
    return 0;
}
