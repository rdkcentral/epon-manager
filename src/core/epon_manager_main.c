/**
 * @file epon_manager_main.c
 * @brief EPON Manager - Main application entry point
 */

#include "controller/eponMgr_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -c, --config FILE    Configuration file path\n");
    printf("  -v, --verbose        Enable console logging\n");
    printf("  -f, --file-log       Enable file logging\n");
    printf("  -t, --cache-ttl SEC  Cache TTL in seconds (default: 30)\n");
    printf("  -h, --help           Show this help message\n");
    printf("\nExample:\n");
    printf("  %s --config /etc/epon_manager.conf --verbose\n", program_name);
}

int main(int argc, char *argv[]) {
    eponMgr_controller_config_t config = {
        .config_file = NULL,
        .enable_console_log = false,
        .enable_file_log = true,
        .cache_ttl_seconds = 30
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config.config_file = argv[++i];
            } else {
                fprintf(stderr, "Error: --config requires a file path\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config.enable_console_log = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file-log") == 0) {
            config.enable_file_log = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--cache-ttl") == 0) {
            if (i + 1 < argc) {
                config.cache_ttl_seconds = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: --cache-ttl requires a value\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("=== EPON Manager Starting ===\n");
    printf("Version: 1.0.0\n");
    printf("Console logging: %s\n", config.enable_console_log ? "enabled" : "disabled");
    printf("File logging: %s\n", config.enable_file_log ? "enabled" : "disabled");
    printf("Cache TTL: %u seconds\n", config.cache_ttl_seconds);
    if (config.config_file) {
        printf("Config file: %s\n", config.config_file);
    }
    printf("\n");
    
    // Initialize controller
    eponMgr_controller_t *controller = eponMgr_controller_init(&config);
    if (!controller) {
        fprintf(stderr, "FATAL: Failed to initialize EPON Manager controller\n");
        return 1;
    }
    
    // Run main event loop (blocks until shutdown)
    int ret = eponMgr_controller_run(controller);
    
    // Cleanup
    eponMgr_controller_destroy(controller);
    
    printf("\n=== EPON Manager Stopped ===\n");
    return ret;
}
