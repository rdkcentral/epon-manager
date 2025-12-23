/**
 * @file eponMgr_logger.h
 * @brief Simple logger for EPON Manager - macro-based logging
 * 
 * Provides simple logging macros for console and file output.
 * No thread-safety or log rotation (handled by RDK logger in production).
 */

#ifndef EPONMGR_LOGGER_H
#define EPONMGR_LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>

/**
 * Log levels
 */
typedef enum {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_INFO  = 3,
    LOG_LEVEL_DEBUG = 4
} epon_log_level_t;

/**
 * Initialize logger
 * @param log_dir Directory for log files (e.g., "./logs/")
 * @param log_level Minimum log level to output
 * @return 0 on success, -1 on error
 */
int eponMgr_logger_init(const char *log_dir, epon_log_level_t log_level);

/**
 * Close logger and cleanup
 */
void eponMgr_logger_close(void);

/**
 * Write log message
 * @param level Log level
 * @param file Source file name
 * @param line Line number
 * @param func Function name
 * @param format Printf-style format string
 */
void eponMgr_logger_write(epon_log_level_t level, const char *file, int line, 
                       const char *func, const char *format, ...);

/**
 * Logging macros - use these instead of calling eponMgr_logger_write directly
 */
#define EPONMGR_LOG_FATAL(fmt, ...) \
    eponMgr_logger_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_ERROR(fmt, ...) \
    eponMgr_logger_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_WARN(fmt, ...) \
    eponMgr_logger_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_INFO(fmt, ...) \
    eponMgr_logger_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_DEBUG(fmt, ...) \
    eponMgr_logger_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif /* EPONMGR_LOGGER_H */
