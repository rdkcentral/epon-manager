/**
 * @file test_logger.c
 * @brief Test program for EPON Logger
 */

#include "../../src/logger/eponMgr_logger.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    (void)argc;  /* Unused parameter */
    (void)argv;  /* Unused parameter */
    
    printf("=== EPON Logger Test ===\n\n");

    /* Initialize logger */
    printf("Initializing logger...\n");
    if (eponMgr_logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    printf("Logger initialized successfully\n\n");

    /* Test all log levels */
    printf("Testing all log levels:\n\n");
    
    EPONMGR_LOG_DEBUG("This is a DEBUG message - lowest priority");
    EPONMGR_LOG_INFO("This is an INFO message - normal operation");
    EPONMGR_LOG_WARN("This is a WARN message - something unusual");
    EPONMGR_LOG_ERROR("This is an ERROR message - something failed");
    EPONMGR_LOG_FATAL("This is a FATAL message - critical error");

    printf("\n");

    /* Test formatted messages */
    printf("Testing formatted messages:\n\n");
    
    int port = 8080;
    const char *status = "active";
    EPONMGR_LOG_INFO("Server started on port %d with status: %s", port, status);
    
    float temperature = 45.7;
    EPONMGR_LOG_WARN("Temperature is %.1f°C - approaching threshold", temperature);
    
    int error_code = 123;
    EPONMGR_LOG_ERROR("Operation failed with error code: %d", error_code);

    printf("\n");

    /* Test multiple rapid logs */
    printf("Testing rapid logging:\n\n");
    for (int i = 0; i < 5; i++) {
        EPONMGR_LOG_DEBUG("Rapid log message #%d", i + 1);
        usleep(10000); /* 10ms delay */
    }

    printf("\n");

    /* Close logger */
    printf("Closing logger...\n");
    eponMgr_logger_close();
    printf("Logger closed successfully\n\n");

    /* Verify log file was created */
    printf("Check log file at: ./logs/epon_manager.log\n");
    printf("\n=== Test Complete ===\n");

    return 0;
}
