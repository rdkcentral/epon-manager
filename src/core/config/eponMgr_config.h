/**
 * @file eponMgr_config.h
 * @brief EPON Manager configuration management
 * 
 * Simple INI file parser with environment variable support.
 * Thread-safe with pthread_mutex protection for runtime reconfiguration.
 */

#ifndef EPONMGR_CONFIG_H
#define EPONMGR_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/**
 * @brief EPON Manager configuration structure (thread-safe)
 */
typedef struct {
    pthread_mutex_t mutex;               /**< Mutex for thread safety */
    
    /* Cache settings */
    uint32_t cache_ttl_seconds;          /**< Cache TTL in seconds (default: 30) */
    
    /* Logging settings */
    char log_level[16];                  /**< Log level: DEBUG, INFO, WARN, ERROR, FATAL */
    char log_directory[256];             /**< Log directory path */
    
    /* HAL settings */
    bool dpoe_enabled;                   /**< DPoE support enabled */
    
    /* RBUS settings */
    bool use_dummy_rbus;                 /**< Use dummy RBUS for testing */
    
    /* Telemetry settings */
    bool use_dummy_telemetry;            /**< Use dummy telemetry for testing */
    
    /* Interface settings */
    uint32_t event_queue_size;           /**< Event queue max size (default: 100) */
} eponMgr_config_t;

/**
 * @brief Initialize configuration with defaults and mutex
 * @param config Pointer to configuration structure
 * @return 0 on success, -1 on error
 */
int eponMgr_config_init_defaults(eponMgr_config_t *config);

/**
 * @brief Destroy configuration and cleanup mutex
 * @param config Pointer to configuration structure
 */
void eponMgr_config_destroy(eponMgr_config_t *config);

/**
 * @brief Load configuration from INI file
 * @param config Pointer to configuration structure
 * @param ini_file Path to INI configuration file
 * @return 0 on success, -1 on error
 */
int eponMgr_config_load_file(eponMgr_config_t *config, const char *ini_file);

/**
 * @brief Override configuration with environment variables
 * 
 * Checks for these environment variables:
 * - EPON_CACHE_TTL
 * - EPONMGR_LOG_LEVEL
 * - EPONMGR_LOG_DIR
 * - EPON_DPOE_ENABLED
 * - EPON_USE_DUMMY_RBUS
 * - EPON_USE_DUMMY_TELEMETRY
 * - EPON_EVENT_QUEUE_SIZE
 * 
 * @param config Pointer to configuration structure
 */
void eponMgr_config_load_env(eponMgr_config_t *config);

/**
 * @brief Validate configuration values
 * @param config Pointer to configuration structure
 * @return 0 if valid, -1 if invalid
 */
int eponMgr_config_validate(const eponMgr_config_t *config);

/**
 * @brief Print configuration (for debugging)
 * @param config Pointer to configuration structure
 */
void eponMgr_config_print(const eponMgr_config_t *config);

/**
 * @brief Get configuration value by key (simple key-value store)
 * @param key Configuration key (e.g., "epon.interface.enable")
 * @param value Buffer to store the value
 * @param value_size Size of the value buffer
 * @return 0 on success, -1 if key not found
 */
int eponMgr_config_get(const char *key, char *value, size_t value_size);

/**
 * @brief Set configuration value by key (simple key-value store)
 * @param key Configuration key (e.g., "epon.interface.enable")
 * @param value Value to set
 * @return 0 on success, -1 on error
 */
int eponMgr_config_set(const char *key, const char *value);

#endif /* EPONMGR_CONFIG_H */
