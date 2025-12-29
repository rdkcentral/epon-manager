/**
 * @file eponMgr_logger.c
 * @brief RDK Logger wrapper for EPON Manager
 */

#include "eponMgr_logger.h"

/**
 * Initialize logger
 */
int eponMgr_logger_init(void) {
    /* Initialize RDK logger */
    rdk_logger_init("/etc/debug.ini");
    
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, "EPON Manager Logger Initialized\n");
    
    return 0;
}

/**
 * Close logger
 */
void eponMgr_logger_close(void) {
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, "EPON Manager Logger Closed\n");
}
