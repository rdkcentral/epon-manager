/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file epon_manager_main.c
 * @brief EPON Manager - Main application entry point
 */

#include "controller/eponMgr_controller.h"
#include "eponMgr_telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

/**
 * @brief Print command-line usage information
 * @param program_name Name of the executable
 */
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -c, --console        Run in console mode (don't daemonize)\n");
    printf("  -h, --help           Show this help message\n");
    printf("\nConfiguration:\n");
    printf("  Persistence: CCSP PSM (dmsb.eponmanager.*)\n");
    printf("\nExample:\n");
    printf("  %s                   # Run as daemon\n", program_name);
    printf("  %s -c                # Run in foreground\n", program_name);
}

/**
 * @brief Daemonize the process
 * 
 * Similar to wanmanager pattern - fork, create new session, redirect I/O
 */
static void daemonize(void) {
    int fd;
    
    switch (fork()) {
    case 0:
        break;
    case -1:
        fprintf(stderr, "Error daemonizing (fork)! %d - %s\n", errno, strerror(errno));
        exit(1);
    default:
        _exit(0);
    }

    if (setsid() < 0) {
        fprintf(stderr, "Error daemonizing (setsid)! %d - %s\n", errno, strerror(errno));
        exit(1);
    }

#ifndef _DEBUG
    fd = open("/dev/null", O_RDONLY);
    if (fd != 0) {
        dup2(fd, 0);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 1) {
        dup2(fd, 1);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 2) {
        dup2(fd, 2);
        close(fd);
    }
#endif
}

/**
 * @brief Signal handler for graceful shutdown
 * @param signum Signal number received
 */
static void signal_handler(int signum) {
    /* Only async-signal-safe operations here.  Telemetry (mutexes/logging/T2
     * APIs) is NOT async-signal-safe and must not be called from a signal
     * handler; it is emitted in main() once the event loop exits. */
    eponMgr_controller_shutdown();
    (void)signum;
}

/**
 * @brief Create PID file
 */
static int create_pid_file(void) {
    FILE *fd = fopen("/var/tmp/epon_manager.pid", "w+");
    if (!fd) {
        fprintf(stderr, "Failed to create /var/tmp/epon_manager.pid: %s\n", strerror(errno));
        return -1;
    }
    
    fprintf(fd, "%d", getpid());
    fclose(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    bool run_as_daemon = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--console") == 0) {
            run_as_daemon = false;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Daemonize if not running in console mode
    if (run_as_daemon) {
        daemonize();
    }
    
    printf("=== EPON Manager Starting ===\n");
    printf("Mode: %s\n", run_as_daemon ? "daemon" : "console");
    printf("\n");
    
    // Create PID file
    if (create_pid_file() != 0) {
        fprintf(stderr, "Warning: Failed to create PID file\n");
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    printf("Signal handlers registered (SIGINT, SIGTERM)\n");
    
    // Initialize telemetry first so init success/failure can be reported.
    (void)eponMgr_telemetry_init("EponManager");

    // Initialize controller
    eponMgr_controller_t *controller = eponMgr_controller_init();
    if (!controller) {
        fprintf(stderr, "FATAL: Failed to initialize EPON Manager controller\n");
        (void)eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_INIT_FAILURE);
        (void)eponMgr_telemetry_cleanup();
        return 1;
    }

    (void)eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_INIT_SUCCESS);

        // Run main event loop (blocks until shutdown)
    int ret = eponMgr_controller_run(controller);

    // Event loop has exited — emit shutdown telemetry from this normal thread
    // context (safe: no longer in a signal handler).
    (void)eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_SHUTDOWN);

    // Cleanup
    eponMgr_controller_destroy(controller);
    (void)eponMgr_telemetry_cleanup();
    
    printf("\n=== EPON Manager Stopped ===\n");
    return ret;
}
