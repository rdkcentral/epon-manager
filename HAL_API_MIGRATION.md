# EPON HAL API Migration Guide

## Overview
This document describes the changes made to the EPON Manager codebase to accommodate the updated EPON HAL API (epon_hal.h). The HAL update introduced significant changes to alarm handling, OAM logging, and LLID management.

---

## HAL API Changes Summary

### 1. Alarm Structure Redesign

**Old API:**
```c
typedef enum {
    EPON_HAL_ALARM_LOS = 0,
    EPON_HAL_ALARM_LOFI,
    EPON_HAL_ALARM_DYING_GASP,
    EPON_HAL_ALARM_POWER_LOW,
    EPON_HAL_ALARM_POWER_HIGH,
    EPON_HAL_ALARM_TEMPERATURE,
    EPON_HAL_ALARM_FEC_THRESHOLD,
    EPON_HAL_ALARM_LASER_BIAS_CURRENT,
    EPON_HAL_ALARM_SUPPLY_VOLTAGE,
    // ... all alarms in one enum
} epon_hal_alarm_t;

// Callback signature
void (*alarm_callback)(epon_hal_alarm_t alarm, bool is_active);
```

**New API:**
```c
// Standard IEEE 802.3ah alarms (LOFI, ERROR_*, OAM_SESSION_LOST, EQUIPMENT_FAILURE)
typedef enum {
    EPON_HAL_ALARM_LOFI = 0,
    EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD,
    EPON_HAL_ALARM_ERROR_FRAME,
    EPON_HAL_ALARM_ERROR_FRAME_PERIOD,
    EPON_HAL_ALARM_ERROR_FRAME_SECONDS,
    EPON_HAL_ALARM_OAM_SESSION_LOST,
    EPON_HAL_ALARM_EQUIPMENT_FAILURE,
    EPON_HAL_ALARM_MAX
} epon_hal_alarm_t;

// Vendor-specific alarms (LOS, DYING_GASP, POWER_*, TEMPERATURE, FEC_THRESHOLD, etc.)
typedef enum {
    EPON_VENDOR_ALARM_LOS = 0,
    EPON_VENDOR_ALARM_DYING_GASP,
    EPON_VENDOR_ALARM_POWER_LOW,
    EPON_VENDOR_ALARM_POWER_HIGH,
    EPON_VENDOR_ALARM_TEMPERATURE,
    EPON_VENDOR_ALARM_FEC_THRESHOLD,
    EPON_VENDOR_ALARM_LASER_BIAS_CURRENT,
    EPON_VENDOR_ALARM_SUPPLY_VOLTAGE,
    EPON_VENDOR_ALARM_MAX
} epon_vendor_alarm_t;

// Alarm type discriminator
typedef enum {
    EPON_ALARM_TYPE_STANDARD = 0,
    EPON_ALARM_TYPE_VENDOR_SPECIFIC = 1
} epon_alarm_type_t;

// Unified alarm structure
typedef struct {
    epon_alarm_type_t alarm_type;
    union {
        epon_hal_alarm_t standard_alarm;
        epon_vendor_alarm_t vendor_alarm;
    };
    uint16_t llid;          // LLID associated with alarm (use EPON_LLID_NOT_APPLICABLE if N/A)
    bool is_active;         // True if alarm raised, false if cleared
} epon_alarm_info_t;

// New callback signature
void (*alarm_callback)(epon_alarm_info_t *alarm_info);
```

**Key Changes:**
- Alarms are now separated into **standard** (IEEE 802.3ah) and **vendor-specific** (DPoE)
- New `epon_alarm_info_t` structure provides unified interface
- LLID association added for multi-LLID scenarios
- Callback now receives pointer to alarm info structure

### 2. LLID Constants Addition

**New Constant:**
```c
#define EPON_LLID_NOT_APPLICABLE 0xFFFF  // Use for alarms not tied to specific LLID
```

### 3. OAM Logging API

**Old API:**
```c
typedef enum {
    EPON_OAM_INFO           = (1 << 0),
    EPON_OAM_EVENT          = (1 << 1),
    EPON_OAM_VAR_REQUEST    = (1 << 2),
    EPON_OAM_VAR_RESPONSE   = (1 << 3),
    EPON_OAM_LOOPBACK       = (1 << 4),
    EPON_OAM_ORG_SPECIFIC   = (1 << 5),
    EPON_OAM_MPCP_REGISTER  = (1 << 6),
    EPON_OAM_MPCP_GATE      = (1 << 7),      // REMOVED - not available in hardware
    EPON_OAM_MPCP_REPORT    = (1 << 8),      // REMOVED - not available in hardware
    EPON_OAM_MPCP_REGISTER_ACK = (1 << 9),
    EPON_OAM_ALL            = 0xFFFFFFFF
} epon_oam_log_type_t;
```

**New API:**
```c
typedef enum {
    EPON_OAM_INFO           = (1 << 0),
    EPON_OAM_EVENT          = (1 << 1),
    EPON_OAM_VAR_REQUEST    = (1 << 2),
    EPON_OAM_VAR_RESPONSE   = (1 << 3),
    EPON_OAM_LOOPBACK       = (1 << 4),
    EPON_OAM_MPCP_REGISTER  = (1 << 5),      // Bit position changed from 6 to 5
    EPON_OAM_MPCP_REGISTER_ACK = (1 << 6),   // Bit position changed from 9 to 6
    EPON_OAM_ALL            = 0xFFFFFFFF
} epon_oam_log_type_t;
```

**Key Changes:**
- Removed `EPON_OAM_MPCP_GATE` and `EPON_OAM_MPCP_REPORT` (not available in software)
- Removed `EPON_OAM_ORG_SPECIFIC` (use VAR_REQUEST/VAR_RESPONSE instead)
- Bit positions renumbered

### 4. HAL Logging API

**New Log Level Added:**
```c
typedef enum {
    HAL_LOG_LEVEL_FATAL = 0,
    HAL_LOG_LEVEL_ERROR,
    HAL_LOG_LEVEL_WARN,
    HAL_LOG_LEVEL_NOTICE,
    HAL_LOG_LEVEL_INFO,
    HAL_LOG_LEVEL_OAM,        // NEW - dedicated level for OAM messages
    HAL_LOG_LEVEL_DEBUG,
    HAL_LOG_LEVEL_TRACE
} hal_log_level_t;
```

---

## Code Changes Made

### 1. Event Queue Structure (`src/core/data_structures/eponMgr_queue.h`)

**Changed:**
```c
// OLD
struct {
    epon_hal_alarm_t alarm;
    bool is_active;
} alarm;

// NEW
struct {
    epon_alarm_info_t alarm_info;
} alarm;
```

**Impact:** Simplified event structure to use new unified alarm info structure.

---

### 2. Controller Alarm Handling (`src/core/controller/eponMgr_controller.c`)

#### A. Alarm Callback Function

**Changed:**
```c
// OLD
static void hal_alarm_callback(epon_hal_alarm_t alarm, bool is_active) {
    event.data.alarm.alarm = alarm;
    event.data.alarm.is_active = is_active;
}

// NEW
static void hal_alarm_callback(epon_alarm_info_t *alarm_info) {
    if (!alarm_info) return;
    event.data.alarm.alarm_info = *alarm_info;
}
```

#### B. Alarm Name Mapping

**Changed:** Split single function into two:

**OLD:**
```c
static const char* get_alarm_name(epon_hal_alarm_t alarm) {
    // Returned names for all alarms (LOS, LOFI, DYING_GASP, etc.)
}
```

**NEW:**
```c
static const char* get_standard_alarm_name(epon_hal_alarm_t alarm) {
    switch (alarm) {
        case EPON_HAL_ALARM_LOFI:
            return "LOFI";
        case EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD:
            return "ERROR_SYMBOL_PERIOD";
        case EPON_HAL_ALARM_ERROR_FRAME:
            return "ERROR_FRAME";
        case EPON_HAL_ALARM_ERROR_FRAME_PERIOD:
            return "ERROR_FRAME_PERIOD";
        case EPON_HAL_ALARM_ERROR_FRAME_SECONDS:
            return "ERROR_FRAME_SECONDS";
        case EPON_HAL_ALARM_OAM_SESSION_LOST:
            return "OAM_SESSION_LOST";
        case EPON_HAL_ALARM_EQUIPMENT_FAILURE:
            return "EQUIPMENT_FAILURE";
        default:
            return "UNKNOWN";
    }
}

static const char* get_vendor_alarm_name(epon_vendor_alarm_t alarm) {
    switch (alarm) {
        case EPON_VENDOR_ALARM_LOS:
            return "LOS";
        case EPON_VENDOR_ALARM_DYING_GASP:
            return "DYING_GASP";
        case EPON_VENDOR_ALARM_POWER_LOW:
            return "POWER_LOW";
        case EPON_VENDOR_ALARM_POWER_HIGH:
            return "POWER_HIGH";
        case EPON_VENDOR_ALARM_TEMPERATURE:
            return "TEMPERATURE";
        case EPON_VENDOR_ALARM_FEC_THRESHOLD:
            return "FEC_THRESHOLD";
        case EPON_VENDOR_ALARM_LASER_BIAS_CURRENT:
            return "LASER_BIAS_CURRENT";
        case EPON_VENDOR_ALARM_SUPPLY_VOLTAGE:
            return "SUPPLY_VOLTAGE";
        default:
            return "UNKNOWN_VENDOR";
    }
}
```

#### C. Alarm Processing Function

**Changed:**
```c
// OLD
static void process_alarm_event(eponMgr_controller_t *ctrl, 
                               epon_hal_alarm_t alarm, 
                               bool is_active) {
    const char *alarm_str = get_alarm_name(alarm);
    if (is_active) {
        EPONMGR_LOG_WARN("Alarm RAISED: %s (%d)\n", alarm_str, alarm);
    } else {
        EPONMGR_LOG_INFO("Alarm CLEARED: %s (%d)\n", alarm_str, alarm);
    }
}

// NEW
static void process_alarm_event(eponMgr_controller_t *ctrl, 
                               epon_alarm_info_t *alarm_info) {
    if (!alarm_info) return;
    
    const char *alarm_str;
    const char *type_str;
    
    if (alarm_info->alarm_type == EPON_ALARM_TYPE_STANDARD) {
        alarm_str = get_standard_alarm_name(alarm_info->standard_alarm);
        type_str = "Standard";
    } else {
        alarm_str = get_vendor_alarm_name(alarm_info->vendor_alarm);
        type_str = "Vendor";
    }
    
    bool is_active = alarm_info->is_active;
    uint16_t llid = alarm_info->llid;
    
    if (is_active) {
        if (llid == EPON_LLID_NOT_APPLICABLE) {
            EPONMGR_LOG_WARN("%s Alarm RAISED: %s\n", type_str, alarm_str);
        } else {
            EPONMGR_LOG_WARN("%s Alarm RAISED: %s (LLID=%u)\n", type_str, alarm_str, llid);
        }
    } else {
        if (llid == EPON_LLID_NOT_APPLICABLE) {
            EPONMGR_LOG_INFO("%s Alarm CLEARED: %s\n", type_str, alarm_str);
        } else {
            EPONMGR_LOG_INFO("%s Alarm CLEARED: %s (LLID=%u)\n", type_str, alarm_str, llid);
        }
    }
}
```

#### D. Event Processing

**Changed:**
```c
// OLD
case EPONMGR_EVENT_TYPE_ALARM:
    process_alarm_event(ctrl, event.data.alarm.alarm, event.data.alarm.is_active);
    break;

// NEW
case EPONMGR_EVENT_TYPE_ALARM:
    process_alarm_event(ctrl, &event.data.alarm.alarm_info);
    break;
```

---

### 3. HAL Mock Test Functions (`tests/hal_mock/epon_hal_mock.c` & `.h`)

#### A. Function Signature

**Changed:**
```c
// OLD
void epon_hal_mock_trigger_alarm(epon_hal_alarm_t alarm, bool is_active);

// NEW
void epon_hal_mock_trigger_alarm(epon_alarm_type_t type, 
                                uint32_t alarm_value, 
                                uint16_t llid, 
                                bool is_active);
```

#### B. Function Implementation

**Changed:**
```c
// OLD
void epon_hal_mock_trigger_alarm(epon_hal_alarm_t alarm, bool is_active) {
    if (g_initialized && g_config.alarm_callback) {
        g_config.alarm_callback(alarm, is_active);
    }
}

// NEW
void epon_hal_mock_trigger_alarm(epon_alarm_type_t type, 
                                uint32_t alarm_value, 
                                uint16_t llid, 
                                bool is_active) {
    if (g_initialized && g_config.alarm_callback) {
        epon_alarm_info_t alarm_info;
        alarm_info.alarm_type = type;
        if (type == EPON_ALARM_TYPE_STANDARD) {
            alarm_info.standard_alarm = (epon_hal_alarm_t)alarm_value;
        } else {
            alarm_info.vendor_alarm = (epon_vendor_alarm_t)alarm_value;
        }
        alarm_info.llid = llid;
        alarm_info.is_active = is_active;
        
        g_config.alarm_callback(&alarm_info);
    }
}
```

---

## Migration Checklist

### Completed ✅

- [x] Updated event queue alarm structure
- [x] Updated alarm callback function signature
- [x] Split alarm name mapping into standard/vendor functions
- [x] Updated alarm processing logic with LLID support
- [x] Updated event dispatcher to pass alarm_info pointer
- [x] Updated HAL mock trigger functions
- [x] Fixed string escaping issues in log statements
- [x] Added BER calculation with both corrected and uncorrectable FEC errors
- [x] Updated max_cpe getter to read from cpe_table directly

### No Changes Required ✅

The following areas use HAL functions but require no changes:
- **Data layer (`eponMgr_data.c`)**: All HAL data structure getters (stats, info, lists) - signatures unchanged
- **TR-181 handlers (`eponMgr_tr181.c`)**: Parameter getters use data layer - no direct HAL calls affected
- **Stats poller (`eponMgr_stats_poller.c`)**: Uses data layer - no direct HAL calls
- **Telemetry (`eponMgr_telemetry.c`)**: Uses data layer - no direct HAL calls

---

## Testing Recommendations

### 1. Alarm Testing
Test both standard and vendor alarms with LLID association:

```bash
# Standard alarm (LOFI) - no LLID
echo "ALARM:0:0:0xFFFF:1" > /tmp/epon_hal_mock.sock

# Vendor alarm (LOS) with LLID 1
echo "ALARM:1:0:1:1" > /tmp/epon_hal_mock.sock

# Vendor alarm (FEC_THRESHOLD) with LLID 2
echo "ALARM:1:5:2:1" > /tmp/epon_hal_mock.sock
```

### 2. Verify Log Output
Expected log formats:
```
Standard Alarm RAISED: LOFI
Vendor Alarm RAISED: LOS (LLID=1)
Vendor Alarm RAISED: FEC_THRESHOLD (LLID=2)
```

### 3. BER Calculation Verification
Verify BER includes both corrected and uncorrectable errors:
```
BER = (fec_corrected + fec_uncorrectable * 8) / (bytes_received * 8)
```

---

## Summary

**Total Files Modified:** 5
1. `src/core/data_structures/eponMgr_queue.h` - Alarm event structure
2. `src/core/controller/eponMgr_controller.c` - Alarm callback, processing, logging
3. `tests/hal_mock/epon_hal_mock.c` - Mock trigger implementation
4. `tests/hal_mock/epon_hal_mock.h` - Mock trigger declaration
5. `src/core/data_structures/eponMgr_statsData.c` - BER calculation improvement

**Key Improvements:**
- ✅ Proper separation of standard vs vendor-specific alarms
- ✅ LLID association for multi-LLID troubleshooting
- ✅ Enhanced logging with alarm type and LLID information
- ✅ Backward compatibility maintained for non-alarm HAL APIs
- ✅ More accurate BER calculation including uncorrectable errors

**No Breaking Changes** to:
- TR-181 parameter interface
- Data layer API
- Stats collection
- RBUS integration
