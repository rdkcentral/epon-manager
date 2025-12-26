/**
 * @file eponMgr_config.c
 * @brief EPON Manager configuration implementation
 */

#include "eponMgr_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Helper to trim whitespace */
static char *trim_whitespace(char *str) {
    char *end;
    
    /* Trim leading space */
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

int eponMgr_config_init_defaults(eponMgr_config_t *config) {
    if (!config) return -1;
    
    memset(config, 0, sizeof(eponMgr_config_t));
    
    /* Initialize mutex */
    if (pthread_mutex_init(&config->mutex, NULL) != 0) {
        return -1;
    }
    
    /* Default values */
    config->cache_ttl_seconds = 30;
    strncpy(config->log_level, "INFO", sizeof(config->log_level) - 1);
    strncpy(config->log_directory, "./logs", sizeof(config->log_directory) - 1);
    config->dpoe_enabled = false;
    config->use_dummy_rbus = true;  /* Default to dummy for local testing */
    config->use_dummy_telemetry = true;
    config->event_queue_size = 100;
    
    return 0;
}

void eponMgr_config_destroy(eponMgr_config_t *config) {
    if (!config) return;
    pthread_mutex_destroy(&config->mutex);
}

int eponMgr_config_load_file(eponMgr_config_t *config, const char *ini_file) {
    FILE *file;
    char line[512];
    char *key, *value;
    char *eq_pos;
    
    if (!config || !ini_file) {
        return -1;
    }
    
    file = fopen(ini_file, "r");
    if (!file) {
        fprintf(stderr, "Failed to open config file: %s\n", ini_file);
        return -1;
    }
    
    printf("Loading configuration from: %s\n", ini_file);
    
    while (fgets(line, sizeof(line), file)) {
        /* Remove newline */
        line[strcspn(line, "\r\n")] = 0;
        
        /* Trim whitespace */
        char *trimmed = trim_whitespace(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        
        /* Skip section headers [section] */
        if (trimmed[0] == '[') {
            continue;
        }
        
        /* Find '=' separator */
        eq_pos = strchr(trimmed, '=');
        if (!eq_pos) {
            continue;
        }
        
        /* Split into key and value */
        *eq_pos = '\0';
        key = trim_whitespace(trimmed);
        value = trim_whitespace(eq_pos + 1);
        
        /* Parse known keys */
        if (strcmp(key, "cache_ttl_seconds") == 0) {
            config->cache_ttl_seconds = (uint32_t)atoi(value);
        }
        else if (strcmp(key, "log_level") == 0) {
            strncpy(config->log_level, value, sizeof(config->log_level) - 1);
        }
        else if (strcmp(key, "log_directory") == 0) {
            strncpy(config->log_directory, value, sizeof(config->log_directory) - 1);
        }
        else if (strcmp(key, "dpoe_enabled") == 0) {
            config->dpoe_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        }
        else if (strcmp(key, "use_dummy_rbus") == 0) {
            config->use_dummy_rbus = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        }
        else if (strcmp(key, "use_dummy_telemetry") == 0) {
            config->use_dummy_telemetry = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        }
        else if (strcmp(key, "event_queue_size") == 0) {
            config->event_queue_size = (uint32_t)atoi(value);
        }
    }
    
    fclose(file);
    return 0;
}

void eponMgr_config_load_env(eponMgr_config_t *config) {
    const char *env_val;
    
    if (!config) return;
    
    /* Check cache TTL */
    env_val = getenv("EPON_CACHE_TTL");
    if (env_val) {
        config->cache_ttl_seconds = (uint32_t)atoi(env_val);
    }
    
    /* Check log level */
    env_val = getenv("EPONMGR_LOG_LEVEL");
    if (env_val) {
        strncpy(config->log_level, env_val, sizeof(config->log_level) - 1);
    }
    
    /* Check log directory */
    env_val = getenv("EPONMGR_LOG_DIR");
    if (env_val) {
        strncpy(config->log_directory, env_val, sizeof(config->log_directory) - 1);
    }
    
    /* Check DPoE */
    env_val = getenv("EPON_DPOE_ENABLED");
    if (env_val) {
        config->dpoe_enabled = (strcmp(env_val, "true") == 0 || strcmp(env_val, "1") == 0);
    }
    
    /* Check dummy RBUS */
    env_val = getenv("EPON_USE_DUMMY_RBUS");
    if (env_val) {
        config->use_dummy_rbus = (strcmp(env_val, "true") == 0 || strcmp(env_val, "1") == 0);
    }
    
    /* Check dummy telemetry */
    env_val = getenv("EPON_USE_DUMMY_TELEMETRY");
    if (env_val) {
        config->use_dummy_telemetry = (strcmp(env_val, "true") == 0 || strcmp(env_val, "1") == 0);
    }
    
    /* Check event queue size */
    env_val = getenv("EPON_EVENT_QUEUE_SIZE");
    if (env_val) {
        config->event_queue_size = (uint32_t)atoi(env_val);
    }
}

int eponMgr_config_validate(const eponMgr_config_t *config) {
    if (!config) {
        return -1;
    }
    
    /* Validate cache TTL */
    if (config->cache_ttl_seconds < 1 || config->cache_ttl_seconds > 300) {
        fprintf(stderr, "Invalid cache_ttl_seconds: %u (must be 1-300)\n", 
                config->cache_ttl_seconds);
        return -1;
    }
    
    /* Validate log level */
    if (strcmp(config->log_level, "DEBUG") != 0 &&
        strcmp(config->log_level, "INFO") != 0 &&
        strcmp(config->log_level, "WARN") != 0 &&
        strcmp(config->log_level, "ERROR") != 0 &&
        strcmp(config->log_level, "FATAL") != 0) {
        fprintf(stderr, "Invalid log_level: %s\n", config->log_level);
        return -1;
    }
    
    /* Validate log directory */
    if (strlen(config->log_directory) == 0) {
        fprintf(stderr, "log_directory cannot be empty\n");
        return -1;
    }
    
    /* Validate event queue size */
    if (config->event_queue_size < 10 || config->event_queue_size > 10000) {
        fprintf(stderr, "Invalid event_queue_size: %u (must be 10-10000)\n",
                config->event_queue_size);
        return -1;
    }
    
    return 0;
}

void eponMgr_config_print(const eponMgr_config_t *config) {
    if (!config) return;
    
    printf("=== EPON Manager Configuration ===\n");
    printf("Cache TTL: %u seconds\n", config->cache_ttl_seconds);
    printf("Log Level: %s\n", config->log_level);
    printf("Log Directory: %s\n", config->log_directory);
    printf("DPoE Enabled: %s\n", config->dpoe_enabled ? "Yes" : "No");
    printf("Use Dummy RBUS: %s\n", config->use_dummy_rbus ? "Yes" : "No");
    printf("Use Dummy Telemetry: %s\n", config->use_dummy_telemetry ? "Yes" : "No");
    printf("Event Queue Size: %u\n", config->event_queue_size);
    printf("==================================\n");
}

/* Simple in-memory key-value store for runtime config */
#define MAX_CONFIG_ENTRIES 100
static struct {
    char key[128];
    char value[256];
} g_config_store[MAX_CONFIG_ENTRIES];
static int g_config_count = 0;
static pthread_mutex_t g_config_store_mutex = PTHREAD_MUTEX_INITIALIZER;

int eponMgr_config_get(const char *key, char *value, size_t value_size) {
    if (!key || !value || value_size == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&g_config_store_mutex);
    
    for (int i = 0; i < g_config_count; i++) {
        if (strcmp(g_config_store[i].key, key) == 0) {
            strncpy(value, g_config_store[i].value, value_size - 1);
            value[value_size - 1] = '\0';
            pthread_mutex_unlock(&g_config_store_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_config_store_mutex);
    return -1;  /* Key not found */
}

int eponMgr_config_set(const char *key, const char *value) {
    if (!key || !value) {
        return -1;
    }
    
    pthread_mutex_lock(&g_config_store_mutex);
    
    /* Check if key already exists */
    for (int i = 0; i < g_config_count; i++) {
        if (strcmp(g_config_store[i].key, key) == 0) {
            /* Update existing entry */
            strncpy(g_config_store[i].value, value, sizeof(g_config_store[i].value) - 1);
            g_config_store[i].value[sizeof(g_config_store[i].value) - 1] = '\0';
            pthread_mutex_unlock(&g_config_store_mutex);
            return 0;
        }
    }
    
    /* Add new entry if space available */
    if (g_config_count < MAX_CONFIG_ENTRIES) {
        strncpy(g_config_store[g_config_count].key, key, sizeof(g_config_store[g_config_count].key) - 1);
        g_config_store[g_config_count].key[sizeof(g_config_store[g_config_count].key) - 1] = '\0';
        
        strncpy(g_config_store[g_config_count].value, value, sizeof(g_config_store[g_config_count].value) - 1);
        g_config_store[g_config_count].value[sizeof(g_config_store[g_config_count].value) - 1] = '\0';
        
        g_config_count++;
        pthread_mutex_unlock(&g_config_store_mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&g_config_store_mutex);
    return -1;  /* No space available */
}
