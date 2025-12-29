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
 * Initialize logger - directly calls rdk_logger_init
 */
static inline int eponMgr_logger_init(void) {
    rdk_logger_init("/etc/debug.ini");
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, "EPON Manager Logger Initialized\n");
    return 0;
}

/**
 * Close logger and cleanup
 */
static inline void eponMgr_logger_close(void) {
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, "EPON Manager Logger Closed\n");
}

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

#endif /* EPONMGR_LOGGER_H */
