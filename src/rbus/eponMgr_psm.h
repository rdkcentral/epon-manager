/**
 * @file eponMgr_psm.h
 * @brief EPON Manager PSM (Persistent Storage Manager) interface
 * 
 * Provides APIs to read/write persistent configuration using direct rbus calls to PSM.
 */

#ifndef EPONMGR_PSM_H
#define EPONMGR_PSM_H

#include <stdbool.h>
#include <stdint.h>
#include <rbus/rbus.h>

/* PSM parameter names for EPON Manager */
#define PSM_EPON_DPOE_ENABLE       "dmsb.eponmanager.DpoeEnable"
#define PSM_EPON_CACHE_TTL         "dmsb.eponmanager.CacheTtlSeconds"

/**
 * @brief Initialize PSM connection
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_init(void);

/**
 * @brief Close PSM connection
 */
void eponMgr_psm_close(void);

/**
 * @brief Read string value from PSM
 * @param param PSM parameter name
 * @param value Buffer to store value
 * @param value_size Size of value buffer
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_string(const char *param, char *value, size_t value_size);

/**
 * @brief Read unsigned integer value from PSM
 * @param param PSM parameter name
 * @param value Pointer to store value
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_uint(const char *param, uint32_t *value);

/**
 * @brief Read boolean value from PSM
 * @param param PSM parameter name
 * @param value Pointer to store value
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_get_bool(const char *param, bool *value);

/**
 * @brief Write string value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_string(const char *param, const char *value);

/**
 * @brief Write unsigned integer value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_uint(const char *param, uint32_t value);

/**
 * @brief Write boolean value to PSM
 * @param param PSM parameter name
 * @param value Value to write
 * @return 0 on success, -1 on error
 */
int eponMgr_psm_set_bool(const char *param, bool value);

#endif /* EPONMGR_PSM_H */
