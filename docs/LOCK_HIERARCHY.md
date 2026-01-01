# EPON Manager - Lock Hierarchy and Thread Safety

**Date:** January 1, 2026  
**Status:** Refactored and simplified

## Overview

This document defines the mutex lock hierarchy for the EPON Manager to prevent deadlocks and ensure thread safety. After refactoring, we have **11 mutexes** (down from 14) with a clear hierarchy.

---

## Lock Hierarchy (Highest to Lowest)

### **Level 1: Controller Mutex** (Highest Priority)
- **Mutex:** `eponMgr_controller_t->mutex`
- **File:** [src/core/controller/eponMgr_controller.c](../src/core/controller/eponMgr_controller.c)
- **Purpose:** Protects controller state and coordinates access to HAL wrapper
- **Lock Functions:**
  - `eponMgr_controller_lock_hal_wrapper()` - Acquires controller mutex
  - `eponMgr_controller_unlock_hal_wrapper()` - Releases controller mutex
  
**Usage Pattern:**
```c
eponMgr_hal_wrapper_t *wrapper = eponMgr_controller_lock_hal_wrapper();
// ... use wrapper and its data structures ...
eponMgr_controller_unlock_hal_wrapper();
```

**RULE:** Never hold any data structure mutex while attempting to acquire controller mutex.

---

### **Level 2: HAL Wrapper Mutex**
- **Mutex:** `eponMgr_hal_wrapper_t->mutex`
- **File:** [src/core/hal_wrapper/eponMgr_hal_wrapper.c](../src/core/hal_wrapper/eponMgr_hal_wrapper.c)
- **Purpose:** Protects HAL wrapper internal state and statistics cache
- **Used In:** All `eponMgr_hal_wrapper_*()` functions

**Lock Order:**
```
controller->mutex → hal_wrapper->mutex
```

**Example:**
```c
eponMgr_hal_wrapper_t *wrapper = eponMgr_controller_lock_hal_wrapper();  // Level 1
// HAL wrapper functions internally lock wrapper->mutex (Level 2)
eponMgr_hal_wrapper_get_link_stats(wrapper, &stats);
eponMgr_controller_unlock_hal_wrapper();
```

---

### **Level 3: Data Structure Mutexes**
These mutexes protect individual data structures and are **peer-level** (no ordering between them).

#### **3.1. CPE List Mutex**
- **Mutex:** `eponMgr_cpe_list_t->mutex`
- **File:** [src/core/data_structures/eponMgr_cpe_list.c](../src/core/data_structures/eponMgr_cpe_list.c)
- **Purpose:** Thread-safe CPE MAC address table operations

#### **3.2. Interface List Mutex**
- **Mutex:** `eponMgr_interface_list_t->mutex`
- **File:** [src/core/data_structures/eponMgr_interface_list.c](../src/core/data_structures/eponMgr_interface_list.c)
- **Purpose:** Thread-safe interface status list operations

#### **3.3. LLID List Mutex**
- **Mutex:** `eponMgr_llid_list_t->mutex`
- **File:** [src/core/data_structures/eponMgr_llid_list.c](../src/core/data_structures/eponMgr_llid_list.c)
- **Purpose:** Thread-safe LLID information list operations

#### **3.4. ONU State Mutex**
- **Mutex:** `eponMgr_onu_state_t->mutex`
- **File:** [src/core/data_structures/eponMgr_onu_state.c](../src/core/data_structures/eponMgr_onu_state.c)
- **Purpose:** Thread-safe ONU state information access

#### **3.5. Queue Mutex**
- **Mutex:** `eponMgr_queue_t->mutex`
- **File:** [src/core/data_structures/eponMgr_queue.c](../src/core/data_structures/eponMgr_queue.c)
- **Purpose:** Thread-safe event queue push/pop operations
- **Note:** All read operations (`is_empty()`, `is_full()`, `size()`) now properly lock

#### **3.6. Stats Data Mutex**
- **Mutex:** `eponMgr_statsData_t->mutex`
- **File:** [src/core/data_structures/eponMgr_statsData.c](../src/core/data_structures/eponMgr_statsData.c)
- **Purpose:** Thread-safe statistics cache operations

**RULE:** Level 3 mutexes are peers - never hold two Level 3 mutexes simultaneously.

**Typical Access Pattern:**
```c
eponMgr_hal_wrapper_t *wrapper = eponMgr_controller_lock_hal_wrapper();  // Level 1
// wrapper->mutex is locked internally by HAL wrapper functions (Level 2)
// Data structure functions lock their own mutexes (Level 3)
eponMgr_llid_list_count(wrapper->llid_list);  // Locks llid_list->mutex
eponMgr_controller_unlock_hal_wrapper();
```

---

### **Level 4: Independent Mutexes** (Lowest Priority)

#### **4.1. Stats Poller Mutex**
- **Mutex:** `eponMgr_stats_poller_t->mutex`
- **File:** [src/core/stats_poller/eponMgr_stats_poller.c](../src/core/stats_poller/eponMgr_stats_poller.c)
- **Purpose:** Controls stats poller thread lifecycle

#### **4.2. Telemetry Mutex**
- **Mutex:** `g_telem_state.mutex` (global static)
- **File:** [src/telemetry/eponMgr_telemetry.c](../src/telemetry/eponMgr_telemetry.c)
- **Purpose:** Guards telemetry module initialization

#### **4.3. Event Condition Variable Mutex**
- **Mutex:** `eponMgr_controller_t->event_mutex`
- **File:** [src/core/controller/eponMgr_controller.c](../src/core/controller/eponMgr_controller.c)
- **Purpose:** Protects condition variable for event listener thread
- **Note:** Only used with `pthread_cond_wait()`

**RULE:** Level 4 mutexes are independent and rarely interact with other locks.

---

## Removed Mutexes (Refactoring)

### **Removed in 2026-01-01 Refactoring:**

1. **`g_llid_table_mutex`** ❌ REMOVED
   - **Reason:** Redundant with controller mutex; TR-181 handlers already hold controller lock
   - **Replaced with:** Controller mutex protection

2. **`g_cpe_table_mutex`** ❌ REMOVED
   - **Reason:** Same as above
   - **Replaced with:** Controller mutex protection

3. **`g_veip_table_mutex`** ❌ REMOVED
   - **Reason:** Had problematic unlock/relock pattern; redundant with controller mutex
   - **Replaced with:** Controller mutex protection and simplified logic

**Impact:** Reduced mutex count by 21%, simplified lock dependencies, eliminated nested lock anti-patterns.

---

## Locking Rules

### **Rule 1: Lock Ordering**
Always acquire locks from **higher level to lower level**:
```
controller → hal_wrapper → data_structure
```

**NEVER:**
```
data_structure → controller  ❌ DEADLOCK RISK
```

### **Rule 2: No Peer Lock Holding**
Never hold two Level 3 mutexes simultaneously:
```c
// BAD:
pthread_mutex_lock(&llid_list->mutex);
pthread_mutex_lock(&cpe_list->mutex);    // ❌ FORBIDDEN
```

```c
// GOOD:
pthread_mutex_lock(&llid_list->mutex);
// ... work ...
pthread_mutex_unlock(&llid_list->mutex);

pthread_mutex_lock(&cpe_list->mutex);    // ✅ OK
// ... work ...
pthread_mutex_unlock(&cpe_list->mutex);
```

### **Rule 3: Short Critical Sections**
Minimize time holding locks:
- Read data quickly
- Release lock before calling functions that may block
- Never hold lock across RPC calls or file I/O

### **Rule 4: Fixed Lock Duration**
Avoid unlock/relock patterns within the same function:
```c
// BAD (old VEIP sync function):
pthread_mutex_lock(&mutex);
// work
pthread_mutex_unlock(&mutex);  // Release
// call another function
pthread_mutex_lock(&mutex);    // Reacquire ❌

// GOOD:
pthread_mutex_lock(&mutex);
// work
pthread_mutex_unlock(&mutex);
// call another function (no reacquire)
```

---

## Thread Safety Guarantees

### **Thread-Safe Operations:**

1. **All data structure operations** - Each has internal mutex
2. **HAL wrapper operations** - Protected by wrapper mutex
3. **Controller state access** - Protected by controller mutex
4. **Event queue operations** - Now properly locked (fixed in refactoring)

### **Not Thread-Safe (By Design):**

1. **Initialization/Shutdown** - Single-threaded by design
2. **Global table arrays** (g_llid_instances, etc.) - Protected by controller mutex, accessed from RBUS handlers only

---

## Common Access Patterns

### **Pattern 1: TR-181 GET Handler**
```c
static rbusError_t handler(rbusHandle_t handle, rbusProperty_t property, ...) {
    // 1. Lock controller
    eponMgr_hal_wrapper_t *wrapper = eponMgr_controller_lock_hal_wrapper();
    
    // 2. Access HAL wrapper (locks wrapper->mutex internally)
    epon_hal_link_stats_t stats;
    eponMgr_hal_wrapper_get_link_stats(wrapper, &stats);
    
    // 3. Access data structures (locks their mutex internally)
    uint32_t count = eponMgr_llid_list_count(wrapper->llid_list);
    
    // 4. Release controller
    eponMgr_controller_unlock_hal_wrapper();
    
    // 5. Return value
    return RBUS_ERROR_SUCCESS;
}
```

### **Pattern 2: HAL Callback → Event Queue**
```c
void hal_callback(epon_onu_status_t status, void *context) {
    eponMgr_controller_t *ctrl = (eponMgr_controller_t *)context;
    
    // Push event (locks queue->mutex internally)
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
    event.data.onu_status.status = status;
    eponMgr_queue_push(ctrl->event_queue, &event);
    
    // Signal event thread (locks event_mutex)
    pthread_mutex_lock(&ctrl->event_mutex);
    pthread_cond_signal(&ctrl->event_cond);
    pthread_mutex_unlock(&ctrl->event_mutex);
}
```

### **Pattern 3: Event Listener Thread**
```c
void *event_listener_thread(void *arg) {
    while (running) {
        // Wait for events (unlocks event_mutex while waiting)
        pthread_mutex_lock(&ctrl->event_mutex);
        pthread_cond_wait(&ctrl->event_cond, &ctrl->event_mutex);
        pthread_mutex_unlock(&ctrl->event_mutex);
        
        // Process events (locks queue->mutex internally)
        eponMgr_event_t event;
        while (eponMgr_queue_pop(ctrl->event_queue, &event) == 0) {
            process_event(ctrl, &event);
        }
    }
}
```

---

## Debugging Tips

### **Enable Lock Debugging (Debug Builds)**

Add to your debug build:
```c
#ifdef DEBUG_LOCKS
#define LOCK_DEBUG(msg, ...) \
    fprintf(stderr, "[LOCK] %s:%d " msg "\\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOCK_DEBUG(msg, ...)
#endif
```

### **Detect Deadlocks with Timeouts**

Use `pthread_mutex_timedlock()` in debug builds:
```c
#ifdef DEBUG_LOCKS
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 5;  // 5 second timeout

if (pthread_mutex_timedlock(&mutex, &timeout) != 0) {
    EPONMGR_LOG_FATAL("Deadlock detected acquiring mutex at %s:%d\\n", 
                      __FILE__, __LINE__);
    abort();
}
#else
pthread_mutex_lock(&mutex);
#endif
```

### **Thread Sanitizer**

Run tests with ThreadSanitizer:
```bash
gcc -fsanitize=thread -g -o epon_manager *.c
./epon_manager
```

---

## Testing Lock Safety

### **Unit Tests**
- Test each data structure with concurrent access
- Verify no data corruption under load

### **Stress Tests**
- Hammer RBUS handlers from multiple threads
- Trigger HAL callbacks rapidly
- Fill event queue and verify no drops

### **Integration Tests**
- Run full system with all threads active
- Monitor for deadlocks
- Verify proper cleanup on shutdown

---

## Change Log

### **2026-01-01: Major Refactoring**
- **Removed** 3 global table mutexes (g_llid_table_mutex, g_cpe_table_mutex, g_veip_table_mutex)
- **Fixed** lockless reads in queue.c (is_empty, is_full, size now properly lock)
- **Refactored** VEIP sync function to eliminate unlock/relock anti-pattern
- **Simplified** lock hierarchy from 4 levels to 4 levels with 11 mutexes (down from 14)
- **Documented** all lock ordering rules and access patterns

### **Key Benefits:**
- Reduced complexity
- Eliminated deadlock risks from nested global table mutexes
- Fixed thread safety bugs in queue operations
- Clearer lock ownership and hierarchy

---

## References

- **POSIX Threads:** https://pubs.opengroup.org/onlinepubs/9699919799/
- **Deadlock Prevention:** "The Art of Multiprocessor Programming" by Herlihy & Shavit
- **Lock-Free Algorithms:** When appropriate, consider atomic operations instead of mutexes

---

**Maintained by:** EPON Manager Development Team  
**Last Updated:** January 1, 2026
