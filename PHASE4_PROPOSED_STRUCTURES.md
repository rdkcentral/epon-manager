# Phase 4 Proposed Data Structures

## 1. Interface Tracking

### Purpose
Track active EPON interfaces (veip0, veip1, etc.) for WanManager PHY status notifications.

### Structure
```c
/**
 * @brief Interface state information
 */
typedef struct {
    char interface_name[32];              /**< Interface name (e.g., "veip0") */
    uint32_t llid;                        /**< Logical Link ID */
    bool is_up;                           /**< Interface operational status */
    time_t last_state_change;             /**< Timestamp of last state change */
    epon_onu_interface_info_t info;       /**< Full interface info from HAL */
} eponMgr_interface_state_t;

/**
 * @brief Interface list manager
 */
typedef struct {
    pthread_mutex_t mutex;                /**< Mutex for thread safety */
    eponMgr_interface_state_t interfaces[EPON_MAX_INTERFACES];  /**< Array of interfaces */
    uint32_t active_count;                /**< Number of UP interfaces */
    uint32_t total_count;                 /**< Total registered interfaces */
} eponMgr_interface_list_t;
```

### Key Functions
```c
void eponMgr_interface_list_init(eponMgr_interface_list_t *list);
void eponMgr_interface_list_destroy(eponMgr_interface_list_t *list);

int eponMgr_interface_add(eponMgr_interface_list_t *list, const char *name, uint32_t llid);
int eponMgr_interface_remove(eponMgr_interface_list_t *list, const char *name);
int eponMgr_interface_update_status(eponMgr_interface_list_t *list, const char *name, bool is_up);

bool eponMgr_interface_is_any_up(eponMgr_interface_list_t *list);
bool eponMgr_interface_is_all_down(eponMgr_interface_list_t *list);
uint32_t eponMgr_interface_get_active_count(eponMgr_interface_list_t *list);
```

### WanManager Notification Logic
```c
/**
 * Called from interface_status_callback
 */
void handle_interface_status_change(const char *interface_name, bool new_status) {
    bool was_any_up = eponMgr_interface_is_any_up(&g_interface_list);
    
    eponMgr_interface_update_status(&g_interface_list, interface_name, new_status);
    
    bool is_any_up = eponMgr_interface_is_any_up(&g_interface_list);
    
    // Notify WanManager only on PHY state change
    if (!was_any_up && is_any_up) {
        // Transition: ALL DOWN → ANY UP
        wanmanager_notify_phy_status("epon0", true);
    }
    else if (was_any_up && !is_any_up) {
        // Transition: ANY UP → ALL DOWN
        wanmanager_notify_phy_status("epon0", false);
    }
}
```

---

## 2. LLID Tracking

### Purpose
Track active Logical Link IDs for multi-LLID scenarios.

### Structure
```c
/**
 * @brief LLID state information
 */
typedef struct {
    uint32_t llid;                        /**< Logical Link ID */
    bool is_active;                       /**< LLID active status */
    char associated_interface[32];        /**< Associated interface name */
    uint64_t packets_sent;                /**< Stats counter */
    uint64_t packets_received;            /**< Stats counter */
} eponMgr_llid_state_t;

/**
 * @brief LLID list manager
 */
typedef struct {
    pthread_mutex_t mutex;                /**< Mutex for thread safety */
    eponMgr_llid_state_t llids[EPON_MAX_LLIDS];  /**< Array of LLIDs (max 8) */
    uint32_t active_count;                /**< Number of active LLIDs */
    uint32_t registered_count;            /**< Total registered LLIDs */
} eponMgr_llid_list_t;
```

### Key Functions
```c
void eponMgr_llid_list_init(eponMgr_llid_list_t *list);
void eponMgr_llid_list_destroy(eponMgr_llid_list_t *list);

int eponMgr_llid_register(eponMgr_llid_list_t *list, uint32_t llid, const char *interface);
int eponMgr_llid_unregister(eponMgr_llid_list_t *list, uint32_t llid);
int eponMgr_llid_set_active(eponMgr_llid_list_t *list, uint32_t llid, bool is_active);

eponMgr_llid_state_t* eponMgr_llid_find(eponMgr_llid_list_t *list, uint32_t llid);
uint32_t eponMgr_llid_get_active_count(eponMgr_llid_list_t *list);
```

---

## 3. ONU State Manager

### Purpose
Track ONU registration status and trigger cache invalidation.

### Structure
```c
/**
 * @brief ONU state information
 */
typedef struct {
    pthread_mutex_t mutex;                /**< Mutex for thread safety */
    epon_onu_status_t current_status;     /**< Current ONU status */
    epon_onu_status_t previous_status;    /**< Previous status (for change detection) */
    time_t last_status_change;            /**< Timestamp of last change */
    uint32_t status_change_count;         /**< Number of status changes */
    bool is_registered;                   /**< Quick check for registered state */
} eponMgr_onu_state_t;
```

### Key Functions
```c
void eponMgr_onu_state_init(eponMgr_onu_state_t *state);
void eponMgr_onu_state_destroy(eponMgr_onu_state_t *state);

void eponMgr_onu_state_update(eponMgr_onu_state_t *state, epon_onu_status_t new_status);
bool eponMgr_onu_state_has_changed(eponMgr_onu_state_t *state);
bool eponMgr_onu_state_is_registered(eponMgr_onu_state_t *state);
```

### Cache Invalidation Logic
```c
/**
 * Called from onu_status_callback
 */
void handle_onu_status_change(epon_onu_status_t new_status) {
    bool was_registered = eponMgr_onu_state_is_registered(&g_onu_state);
    
    eponMgr_onu_state_update(&g_onu_state, new_status);
    
    bool is_registered = eponMgr_onu_state_is_registered(&g_onu_state);
    
    // Invalidate ALL cache on status change
    if (was_registered != is_registered) {
        EPONMGR_LOG_INFO("ONU status changed: %d -> %d, invalidating cache",
                         was_registered, is_registered);
        eponMgr_cache_invalidate_all(&g_cache);
    }
}
```

---

## 4. Adding Mutex to Existing Structures

### Cache (eponMgr_cache_t)
```c
typedef struct {
    pthread_mutex_t mutex;                /**< NEW: Mutex for thread safety */
    uint32_t ttl_seconds;
    eponMgr_cache_link_stats_t link_stats;
    eponMgr_cache_transceiver_stats_t transceiver_stats;
    eponMgr_cache_manufacturer_info_t manufacturer_info;
    eponMgr_cache_link_info_t link_info;
} eponMgr_cache_t;

// Init/Destroy
void eponMgr_cache_init(eponMgr_cache_t *cache, uint32_t ttl_seconds) {
    // ...existing code...
    pthread_mutex_init(&cache->mutex, NULL);
}

void eponMgr_cache_destroy(eponMgr_cache_t *cache) {
    if (!cache) return;
    pthread_mutex_destroy(&cache->mutex);
}

// Get/Set with mutex
bool eponMgr_cache_get_link_stats(eponMgr_cache_t *cache, epon_hal_link_stats_t *stats) {
    if (!cache || !stats) return false;
    
    pthread_mutex_lock(&cache->mutex);
    
    bool result = false;
    if (cache->link_stats.valid && 
        eponMgr_cache_is_stats_valid(cache->link_stats.timestamp, cache->ttl_seconds)) {
        memcpy(stats, &cache->link_stats.data, sizeof(epon_hal_link_stats_t));
        result = true;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return result;
}
```

### Queue (eponMgr_queue_t)
```c
typedef struct {
    pthread_mutex_t mutex;                /**< NEW: Mutex for thread safety */
    eponMgr_event_t *events;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} eponMgr_queue_t;

int eponMgr_queue_init(eponMgr_queue_t *queue, uint32_t capacity) {
    // ...existing code...
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->events);
        return -1;
    }
    return 0;
}

void eponMgr_queue_destroy(eponMgr_queue_t *queue) {
    if (!queue) return;
    pthread_mutex_destroy(&queue->mutex);
    // ...existing code...
}

int eponMgr_queue_push(eponMgr_queue_t *queue, const eponMgr_event_t *event) {
    if (!queue || !event) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (eponMgr_queue_is_full_unsafe(queue)) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    memcpy(&queue->events[queue->head], event, sizeof(eponMgr_event_t));
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}
```

### Config (eponMgr_config_t)
```c
typedef struct {
    pthread_mutex_t mutex;                /**< NEW: Mutex for thread safety */
    uint32_t cache_ttl_seconds;
    // ...existing fields...
} eponMgr_config_t;

// Config is typically read-only after initialization, but mutex needed if
// we support runtime reconfiguration via TR-181 SET operations
```

---

## 5. Lock Ordering Rules (Per Implementation Plan)

**Lock Hierarchy:**
```
Config → Cache → Queue → Interface List → LLID List → ONU State
```

**Rules:**
1. Always acquire locks in this order
2. Never acquire in reverse order (deadlock risk)
3. Use `pthread_mutex_timedlock()` with 5s timeout
4. Always unlock on error paths
5. No blocking operations while holding locks

**Example:**
```c
// GOOD: Config → Cache
pthread_mutex_lock(&g_config.mutex);
int ttl = g_config.cache_ttl_seconds;
pthread_mutex_unlock(&g_config.mutex);

pthread_mutex_lock(&g_cache.mutex);
cache_operation_with_ttl(ttl);
pthread_mutex_unlock(&g_cache.mutex);

// BAD: Reverse order - potential deadlock!
pthread_mutex_lock(&g_cache.mutex);
pthread_mutex_lock(&g_config.mutex);  // WRONG ORDER!
```

---

## 6. Phase 4 Implementation Plan

### Step 1: Add Mutex to Existing Structures (Week 1)
- Update `eponMgr_cache_t` with mutex
- Update `eponMgr_queue_t` with mutex
- Update `eponMgr_config_t` with mutex
- Update all get/set functions
- Add unit tests for thread safety

### Step 2: Create Interface List Manager (Week 1)
- Create `src/core/data_structures/eponMgr_interface_list.{c,h}`
- Implement functions
- Add unit tests

### Step 3: Create LLID List Manager (Week 1)
- Create `src/core/data_structures/eponMgr_llid_list.{c,h}`
- Implement functions
- Add unit tests

### Step 4: Create ONU State Manager (Week 2)
- Create `src/core/controller/eponMgr_onu_state.{c,h}`
- Implement cache invalidation logic
- Add unit tests

### Step 5: Create HAL Wrapper (Week 2)
- Create `src/core/hal_wrapper/eponMgr_hal_wrapper.{c,h}`
- Integrate with cache
- Query HAL on cache miss
- Add integration tests with mock HAL

---

## Summary

**What's Missing Now (Phase 3):**
- ❌ Thread safety (mutex)
- ❌ Interface list tracking
- ❌ LLID list tracking
- ❌ ONU state manager

**What We'll Add in Phase 4:**
- ✅ Mutex protection for all structures
- ✅ Interface list with WanManager logic
- ✅ LLID list for multi-LLID support
- ✅ ONU state manager with cache invalidation
- ✅ HAL wrapper with caching integration

**Why Deferred:**
Per implementation plan: *"No thread-safety (will be added in Phase 4 if needed)"*

But you're absolutely right - we MUST add it before production!
