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
 * Logging macros - include function name and line number for better debugging
 */
#define EPONMGR_LOG_FATAL(fmt, ...) \
    RDK_LOG(RDK_LOG_FATAL, EPONMGR_LOG_MODULE, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define EPONMGR_LOG_ERROR(fmt, ...) \
    RDK_LOG(RDK_LOG_ERROR, EPONMGR_LOG_MODULE, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define EPONMGR_LOG_WARN(fmt, ...) \
    RDK_LOG(RDK_LOG_WARN, EPONMGR_LOG_MODULE, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define EPONMGR_LOG_INFO(fmt, ...) \
    RDK_LOG(RDK_LOG_INFO, EPONMGR_LOG_MODULE, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define EPONMGR_LOG_DEBUG(fmt, ...) \
    RDK_LOG(RDK_LOG_DEBUG, EPONMGR_LOG_MODULE, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#endif /* EPONMGR_LOGGER_H */
