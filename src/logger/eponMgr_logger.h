/**
 * @file eponMgr_logger.h
 * @brief RDK Logger wrapper for EPON Manager
 */

#ifndef EPONMGR_LOGGER_H
#define EPONMGR_LOGGER_H

#include <rdk_debug.h>

/* RDK Logger module name */
#define EPONMGR_LOG_MODULE "LOG.RDK.EPONMANAGER"

/**
 * Initialize logger
 * @return 0 on success, -1 on error
 */
int eponMgr_logger_init(void);

/**
 * Close logger and cleanup
 */
void eponMgr_logger_close(void);

/**
 * Logging macros - directly map to RDK_LOG for efficiency
 */
#define EPONMGR_LOG_FATAL(fmt, ...) \
    RDK_LOG(RDK_LOG_FATAL, EPONMGR_LOG_MODULE, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_ERROR(fmt, ...) \
    RDK_LOG(RDK_LOG_ERROR, EPONMGR_LOG_MODULE, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_WARN(fmt, ...) \
    RDK_LOG(RDK_LOG_WARN, EPONMGR_LOG_MODULE, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_INFO(fmt, ...) \
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, fmt, ##__VA_ARGS__)

#define EPONMGR_LOG_DEBUG(fmt, ...) \
    RDK_LOG(RDK_LOG_DEBUG, EPONMGR_LOG_MODULE, fmt, ##__VA_ARGS__)

/**
 * Map HAL logging to EPON Manager RDK logger profile
 * Include this before epon_hal.h to enable HAL logging

#ifndef HAL_LOG_FUNCTION
static inline rdk_LogLevel map_hal_to_rdk_level(hal_log_level_t level) {
    switch (level) {
        case HAL_LOG_LEVEL_FATAL:  return RDK_LOG_FATAL;
        case HAL_LOG_LEVEL_ERROR:  return RDK_LOG_ERROR;
        case HAL_LOG_LEVEL_WARN:   return RDK_LOG_WARN;
        case HAL_LOG_LEVEL_NOTICE: return RDK_LOG_NOTICE;
        case HAL_LOG_LEVEL_INFO:   return RDK_LOG_INFO;
        case HAL_LOG_LEVEL_DEBUG:  return RDK_LOG_DEBUG;
        case HAL_LOG_LEVEL_TRACE:  return RDK_LOG_DEBUG;
        default:                   return RDK_LOG_INFO;
    }
}

#define HAL_LOG_FUNCTION(level, func, line, format, ...) \
    RDK_LOG(map_hal_to_rdk_level(level), EPONMGR_LOG_MODULE, "[%s:%d] " format, func, line, ##__VA_ARGS__)
#endif
 */
#endif /* EPONMGR_LOGGER_H */
