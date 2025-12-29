# Dynamic Statistics Update Feature

## Overview

The EPON HAL Mock now supports dynamic statistics updates via Unix Domain Socket IPC, enabling comprehensive testing of statistics collection and monitoring in EPON Manager without physical hardware.

## Architecture

```
┌────────────────────────────────────────────────────────┐
│  EPON Manager Process                                  │
│                                                        │
│  ┌──────────────────────────────────────────────────┐ │
│  │ libepon_hal_mock.so                              │ │
│  │                                                  │ │
│  │  Internal State:                                │ │
│  │  ┌──────────────────────────────────────────┐  │ │
│  │  │ g_link_stats                             │  │ │
│  │  │ - packets_sent, packets_received         │  │ │
│  │  │ - bytes_sent, bytes_received             │  │ │
│  │  │ - errors_sent, errors_received           │  │ │
│  │  │ - fec_corrected, fec_uncorrectable       │  │ │
│  │  └──────────────────────────────────────────┘  │ │
│  │  ┌──────────────────────────────────────────┐  │ │
│  │  │ g_transceiver_stats                      │  │ │
│  │  │ - transmit_optical_level (tx_power)      │  │ │
│  │  │ - optical_signal_level (rx_power)        │  │ │
│  │  │ - temperature                            │  │ │
│  │  │ - bias_current                           │  │ │
│  │  │ - supply_voltage                         │  │ │
│  │  └──────────────────────────────────────────┘  │ │
│  │                                                  │ │
│  │  Control Socket Thread:                         │ │
│  │  ┌──────────────────────────────────────────┐  │ │
│  │  │ Listens on /tmp/epon_hal_mock.sock       │◄─┼─┼─── Commands
│  │  │ Processes: LINKSTATS, TRANSCVRSTATS,     │  │ │
│  │  │            INCREMENT commands             │  │ │
│  │  └──────────────────────────────────────────┘  │ │
│  │                                                  │ │
│  │  HAL APIs return g_link_stats & g_transceiver   │ │
│  │  ┌──────────────────────────────────────────┐  │ │
│  │  │ epon_hal_get_link_statistics()           │  │ │
│  │  │ epon_hal_get_transceiver_info()          │  │ │
│  │  └──────────────────────────────────────────┘  │ │
│  └──────────────────────────────────────────────────┘ │
│                  ▲                                     │
│                  │ Query                               │
│  ┌───────────────┴──────────────────────────────┐     │
│  │ EPON Manager TR-181 Data Model               │     │
│  │ - Device.Ethernet.Link.1.Stats.*             │     │
│  │ - Device.Optical.Interface.1.*               │     │
│  └──────────────────────────────────────────────┘     │
└────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────┐
│  epon_hal_trigger (CLI Tool)                           │
│                                                        │
│  Commands:                                             │
│  -L packets_sent=1000000      → LINKSTATS:...         │
│  -T tx_power=-2.5             → TRANSCVRSTATS:...     │
│  -I packets_received=5000     → INCREMENT:...         │
└────────────────────────────────────────────────────────┘
```

## New Socket Commands

### 1. LINKSTATS - Absolute Statistics Set

**Format:** `LINKSTATS:<field>:<value>`

**Supported Fields:**
- `packets_sent` - Total packets transmitted
- `packets_received` - Total packets received
- `bytes_sent` - Total bytes transmitted
- `bytes_received` - Total bytes received
- `errors_sent` - Transmit errors
- `errors_received` - Receive errors
- `fec_corrected` - FEC correctable errors
- `fec_uncorrectable` - FEC uncorrectable errors

**Example:**
```bash
epon_hal_trigger -L packets_sent=1000000
# Sends: "LINKSTATS:packets_sent:1000000"
# Result: g_link_stats.packets_sent = 1000000
```

### 2. TRANSCVRSTATS - Transceiver Statistics Set

**Format:** `TRANSCVRSTATS:<field>:<value>`

**Supported Fields:**
- `tx_power` - Transmit optical power in dBm (float)
- `rx_power` - Receive optical signal level in dBm (float)
- `temperature` - Transceiver temperature in °C (float)
- `bias_current` - Laser bias current in mA (float)
- `voltage` - Supply voltage in V (float)

**Example:**
```bash
epon_hal_trigger -T tx_power=-2.5
# Sends: "TRANSCVRSTATS:tx_power:-2.5"
# Result: g_transceiver_stats.transmit_optical_level = -2.5

epon_hal_trigger -T temperature=52.3
# Sends: "TRANSCVRSTATS:temperature:52.3"
# Result: g_transceiver_stats.temperature = 52.3
```

### 3. INCREMENT - Relative Statistics Change

**Format:** `INCREMENT:<field>:<value>`

**Supported Fields:** (Same as LINKSTATS, except FEC fields)
- `packets_sent`, `packets_received`
- `bytes_sent`, `bytes_received`
- `errors_sent`, `errors_received`

**Example:**
```bash
epon_hal_trigger -I packets_sent=5000
# Sends: "INCREMENT:packets_sent:5000"
# Result: g_link_stats.packets_sent += 5000
```

## Use Cases

### 1. Baseline Statistics Setup

Set initial counter values to simulate a device that has been running:

```bash
epon_hal_trigger -L packets_sent=1000000
epon_hal_trigger -L packets_received=980000
epon_hal_trigger -L bytes_sent=1024000000
epon_hal_trigger -L bytes_received=1003520000
```

### 2. Traffic Simulation

Incrementally update counters to simulate ongoing traffic:

```bash
# Simulate 10 seconds of traffic at 5000 pps
for i in {1..10}; do
    epon_hal_trigger -I packets_sent=5000
    epon_hal_trigger -I bytes_sent=7680000  # 5000 packets * 1536 bytes avg
    sleep 1
done
```

### 3. Optical Power Monitoring

Test transceiver monitoring and alarm thresholds:

```bash
# Normal conditions
epon_hal_trigger -T tx_power=-2.5
epon_hal_trigger -T rx_power=-15.8

# Simulate fiber degradation
epon_hal_trigger -T rx_power=-22.3  # Below threshold

# Simulate recovery
epon_hal_trigger -T rx_power=-16.2
```

### 4. FEC Error Injection

Test FEC monitoring and threshold violations:

```bash
# Initial state
epon_hal_trigger -L fec_corrected=100
epon_hal_trigger -L fec_uncorrectable=0

# Simulate line degradation
for i in {1..5}; do
    epon_hal_trigger -I fec_corrected=50
    epon_hal_trigger -I fec_uncorrectable=2
    sleep 1
done

# Final: fec_corrected=350, fec_uncorrectable=10
```

### 5. Temperature Alarm Testing

Simulate environmental stress:

```bash
# Normal temperature
epon_hal_trigger -T temperature=42.0

# Gradual temperature increase
for temp in 45 50 55 60 65 70; do
    epon_hal_trigger -T temperature=$temp
    sleep 2
done

# Should trigger EPON_HAL_ALARM_TEMPERATURE when threshold exceeded
```

### 6. End-to-End Verification

Set statistics and verify TR-181 reads them correctly:

```bash
# Set known values
epon_hal_trigger -L packets_sent=123456
epon_hal_trigger -L bytes_received=789012345

# Query via RBUS
rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsSent"
# Expected: 123456

rbus-cli get "Device.Ethernet.Link.1.Stats.BytesReceived"
# Expected: 789012345
```

## Testing Script

A comprehensive test script is provided: [test_stats_updates.sh](test_stats_updates.sh)

```bash
# Run the test suite
cd tests/hal_mock
./test_stats_updates.sh
```

The script tests:
- ✓ Baseline statistics setup
- ✓ Incremental traffic simulation
- ✓ Optical parameter manipulation
- ✓ FEC error monitoring
- ✓ Error rate testing
- ✓ Temperature alarm scenarios
- ✓ Complete operational lifecycle
- ✓ TR-181/RBUS verification (if available)

## Implementation Details

### Mock State Variables

```c
// In epon_hal_mock.c
static epon_hal_link_stats_t g_link_stats = {0};
static epon_hal_transceiver_stats_t g_transceiver_stats = {0};
```

### Command Processor

```c
static void process_command(const char *command) {
    // ... existing STATUS, ALARM, INTERFACE handlers ...
    
    if (strcmp(type, "LINKSTATS") == 0) {
        char *field = strtok_r(NULL, ":", &saveptr);
        char *value = strtok_r(NULL, ":", &saveptr);
        
        if (strcmp(field, "packets_sent") == 0) {
            g_link_stats.packets_sent = strtoull(value, NULL, 10);
        }
        // ... handle all link stats fields ...
    }
    else if (strcmp(type, "TRANSCVRSTATS") == 0) {
        // Handle transceiver stats with atof() for floating point
    }
    else if (strcmp(type, "INCREMENT") == 0) {
        // Add value to existing counter
    }
}
```

### HAL API Returns

The mock HAL APIs return the internal state:

```c
epon_hal_status_t epon_hal_get_link_statistics(epon_hal_link_stats_t *stats) {
    if (!g_initialized || !stats) {
        return EPON_HAL_STATUS_ERROR;
    }
    memcpy(stats, &g_link_stats, sizeof(epon_hal_link_stats_t));
    return EPON_HAL_STATUS_OK;
}

epon_hal_status_t epon_hal_get_transceiver_info(epon_hal_transceiver_stats_t *info) {
    if (!g_initialized || !info) {
        return EPON_HAL_STATUS_ERROR;
    }
    memcpy(info, &g_transceiver_stats, sizeof(epon_hal_transceiver_stats_t));
    return EPON_HAL_STATUS_OK;
}
```

## Data Flow

1. **Set Statistics:**
   ```
   epon_hal_trigger -L packets_sent=1000
     ↓
   Socket command: "LINKSTATS:packets_sent:1000"
     ↓
   process_command() parses and updates g_link_stats.packets_sent
     ↓
   HAL mock internal state updated
   ```

2. **Query Statistics:**
   ```
   EPON Manager calls epon_hal_get_link_statistics()
     ↓
   Mock returns g_link_stats
     ↓
   TR-181 data model updated via RBUS
     ↓
   rbus-cli can query Device.Ethernet.Link.1.Stats.*
   ```

3. **Increment Statistics:**
   ```
   epon_hal_trigger -I packets_received=100
     ↓
   Socket command: "INCREMENT:packets_received:100"
     ↓
   process_command() adds 100 to g_link_stats.packets_received
     ↓
   Simulates continuous traffic accumulation
   ```

## Integration Points

### 1. TR-181 Data Model

Statistics map to TR-181 parameters:

| HAL Field | TR-181 Parameter |
|-----------|-----------------|
| packets_sent | Device.Ethernet.Link.{i}.Stats.PacketsSent |
| packets_received | Device.Ethernet.Link.{i}.Stats.PacketsReceived |
| bytes_sent | Device.Ethernet.Link.{i}.Stats.BytesSent |
| bytes_received | Device.Ethernet.Link.{i}.Stats.BytesReceived |
| transmit_optical_level | Device.Optical.Interface.{i}.TransmitPower |
| optical_signal_level | Device.Optical.Interface.{i}.ReceivePower |
| temperature | Device.Optical.Interface.{i}.Temperature |

### 2. Telemetry & Monitoring

Test telemetry markers for:
- High packet error rates
- FEC threshold violations
- Optical power out of range
- Temperature alarms

### 3. WanManager Integration

Verify WanManager responds to:
- Link statistics updates
- Interface state changes
- PHY status notifications

## Benefits

1. **Hardware-Independent Testing:** Test statistics collection without physical EPON hardware
2. **Reproducible Scenarios:** Create precise test conditions with known values
3. **Threshold Testing:** Trigger alarm conditions by setting specific values
4. **Performance Testing:** Simulate high traffic rates and error conditions
5. **Integration Validation:** Verify end-to-end data flow from HAL → TR-181 → RBUS
6. **Automated Testing:** Script complex scenarios for CI/CD pipelines

## Limitations

1. **No Real Traffic:** Statistics don't reflect actual network conditions
2. **Manual Updates:** Requires explicit commands to change values
3. **No Rate Limiting:** Can set unrealistic values (e.g., negative packets)
4. **Single Instance:** Statistics shared across all interfaces in mock

## Future Enhancements

1. **Auto-Increment Mode:** Background thread automatically increments counters
2. **Traffic Profiles:** Pre-configured traffic patterns (idle, normal, heavy)
3. **GET Commands:** Query current statistics via socket
4. **Batch Updates:** Set multiple fields in one command
5. **Reset Command:** Clear all statistics to zero
6. **Validation:** Enforce realistic value ranges

## See Also

- [README.md](README.md) - Complete HAL mock documentation
- [test_stats_updates.sh](test_stats_updates.sh) - Comprehensive test script
- [test_trigger_ipc.sh](test_trigger_ipc.sh) - Event trigger examples
- [epon_hal.h](../../include/epon_hal.h) - HAL API definitions
