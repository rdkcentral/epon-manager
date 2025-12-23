/**
 * @file eponMgr_logger.c
 * @brief Simple logger implementation for EPON Manager
 */

#include "eponMgr_logger.h"
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Global state */
static FILE *g_log_file = NULL;
static epon_log_level_t g_log_level = LOG_LEVEL_INFO;
static const char *g_log_dir = NULL;

/* Log level names */
static const char *log_level_names[] = {
    "FATAL",
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG"
};

/**
 * Get current timestamp string
 */
static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * Initialize logger
 */
int eponMgr_logger_init(const char *log_dir, epon_log_level_t log_level) {
    if (!log_dir) {
        fprintf(stderr, "Logger init failed: NULL log directory\n");
        return -1;
    }

    /* Create log directory if it doesn't exist */
    struct stat st = {0};
    if (stat(log_dir, &st) == -1) {
        if (mkdir(log_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create log directory: %s\n", log_dir);
            return -1;
        }
    }

    /* Open log file */
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/epon_manager.log", log_dir);
    
    g_log_file = fopen(log_file_path, "a");
    if (!g_log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
        return -1;
    }

    g_log_level = log_level;
    g_log_dir = log_dir;

    /* Write initialization message */
    fprintf(g_log_file, "\n=== EPON Manager Logger Initialized ===\n");
    fprintf(g_log_file, "Log Level: %s\n", log_level_names[log_level]);
    fprintf(g_log_file, "Log Directory: %s\n", log_dir);
    fflush(g_log_file);

    return 0;
}

/**
 * Close logger
 */
void eponMgr_logger_close(void) {
    if (g_log_file) {
        fprintf(g_log_file, "=== EPON Manager Logger Closed ===\n\n");
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

/**
 * Write log message
 */
void eponMgr_logger_write(epon_log_level_t level, const char *file, int line,
                       const char *func, const char *format, ...) {
    /* Check log level */
    if (level > g_log_level) {
        return;
    }

    /* Get timestamp */
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Extract filename from path */
    const char *filename = strrchr(file, '/');
    if (filename) {
        filename++;
    } else {
        filename = file;
    }

    /* Format message */
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    /* Write to console */
    printf("[%s] [%s] [%s:%d:%s] %s\n", 
           timestamp, log_level_names[level], filename, line, func, message);

    /* Write to file */
    if (g_log_file) {
        fprintf(g_log_file, "[%s] [%s] [%s:%d:%s] %s\n",
                timestamp, log_level_names[level], filename, line, func, message);
        fflush(g_log_file);
    }
}
