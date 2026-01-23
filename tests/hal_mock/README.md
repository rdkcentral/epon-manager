# EPON HAL Mock Library

Mock implementation of the EPON HAL for RDK integration testing.

## Components

### 1. HAL Mock Library (`libepon_hal_mock.so`)

Provides a complete mock implementation of the EPON HAL interface with:
- All standard HAL APIs (init, get_stats, get_info, etc.)
- Mock data for testing TR-181 integration
- **Unix Domain Socket control interface** for IPC at `/tmp/epon_hal_mock.sock`
- Background thread listening for event trigger commands

### 2. Interactive Trigger Utility (`epon_hal_trigger`)

Command-line tool for triggering HAL events on a running EPON Manager instance.
Communicates with EPON Manager via Unix Domain Socket.

## Architecture

```
┌─────────────────────────────────────┐
│     EPON Manager Process            │
│  ┌───────────────────────────────┐  │
│  │  libepon_hal_mock.so          │  │
│  │  ┌─────────────────────────┐  │  │
│  │  │ Control Socket Thread   │  │  │
│  │  │ Listens on:             │  │  │
│  │  │ /tmp/epon_hal_mock.sock │◄─┼──┼─── Commands
│  │  └─────────────────────────┘  │  │
│  │           ↓                    │  │
│  │  ┌─────────────────────────┐  │  │
│  │  │ Registered Callbacks    │  │  │
│  │  │ - status_callback()     │  │  │
│  │  │ - alarm_callback()      │  │  │
│  │  │ - interface_callback()  │  │  │
│  │  └─────────────────────────┘  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│   epon_hal_trigger (CLI Tool)       │
│                                     │
│  Connects to socket and sends:     │
│  - STATUS:<value>                  │
│  - ALARM:<type>:<active>           │
│  - INTERFACE:<name>:<status>       │
└─────────────────────────────────────┘
```

## Using the Trigger Utility

The `epon_hal_trigger` utility allows you to simulate HAL events for testing EPON Manager without physical hardware.

### Basic Usage

```bash
# Trigger ONU registration
epon_hal_trigger --status register

# Trigger LOS alarm
epon_hal_trigger --alarm los

# Clear LOS alarm
epon_hal_trigger --alarm los --clear

# Trigger interface link up
epon_hal_trigger --interface veip0:up

# Trigger interface link down
epon_hal_trigger --interface veip0:down
```

### Statistics Manipulation

```bash
# Set absolute statistics values
epon_hal_trigger --linkstats packets_sent=1000000
epon_hal_trigger --linkstats bytes_received=52428800

# Set transceiver optical stats
epon_hal_trigger --transcvrstats tx_power=-2.5
epon_hal_trigger --transcvrstats rx_power=-15.3
epon_hal_trigger --transcvrstats temperature=45.2

# Increment statistics (simulate traffic)
epon_hal_trigger --increment packets_sent=5000
epon_hal_trigger --increment bytes_received=32768

# Multiple stats updates in sequence
epon_hal_trigger --linkstats fec_corrected=100
epon_hal_trigger --linkstats fec_uncorrectable=5
epon_hal_trigger --linkstats errors_received=10
```

### Advanced Usage

```bash
# Repeat event 5 times with 500ms delay
epon_hal_trigger -s los -r 5 -d 500

# Simulate registration sequence
epon_hal_trigger -s signal
sleep 1
epon_hal_trigger -s register
sleep 1
epon_hal_trigger -i veip0:up

# Simulate link flap
for i in {1..10}; do
    epon_hal_trigger -i veip0:down -d 100
    epon_hal_trigger -i veip0:up -d 100
done
```

### Available Events

#### ONU Status Events
- `los` - Loss of signal (PHY down)
- `signal` - Downstream signal detected
- `register` - LLID-0 online (registered)
- `deregister` - LLID-0 offline (deregistered)

#### Alarm Types
- `los` - Loss of signal
- `lofi` - Loss of frame/lock
- `dying_gasp` - Imminent power loss
- `error_symbol` - Errored symbol period
- `error_frame` - Errored frame
- `oam_lost` - OAM session lost
- `power_low` - Optical power below threshold
- `power_high` - Optical power above threshold
- `equipment` - Equipment failure
- `temperature` - Temperature threshold exceeded
- `fec` - FEC uncorrectable errors
- `laser_bias` - Laser bias current out of range
- `voltage` - Supply voltage out of range

#### Interface Status
Format: `<interface_name>:<up|down>`
- Examples: `veip0:up`, `veip0:down`, `veip1:up`

### Testing Scenarios

#### 1. ONU Bringup
```bash
#!/bin/bash
echo "Simulating ONU registration sequence..."
epon_hal_trigger -s signal
sleep 2
epon_hal_trigger -s register
sleep 1
epon_hal_trigger -i veip0:up
echo "ONU online"
```

#### 2. Alarm Storm
```bash
#!/bin/bash
echo "Simulating multiple alarms..."
epon_hal_trigger -a power_low
epon_hal_trigger -a temperature
epon_hal_trigger -a fec
sleep 5
echo "Clearing alarms..."
epon_hal_trigger -a power_low -c
epon_hal_trigger -a temperature -c
epon_hal_trigger -a fec -c
```

#### 3. Link Failure Recovery
```bash
#!/bin/bash
echo "Simulating link failure..."
epon_hal_trigger -a los
epon_hal_trigger -i veip0:down
epon_hal_trigger -s deregister
sleep 3
echo "Recovering link..."
epon_hal_trigger -a los -c
epon_hal_trigger -s signal
sleep 1
epon_hal_trigger -s register
epon_hal_trigger -i veip0:up
echo "Link recovered"
```

#### 4. Statistics Validation
```bash
#!/bin/bash
# Simulate active traffic and verify TR-181 reads updated values

echo "Setting baseline statistics..."
epon_hal_trigger -L packets_sent=1000000
epon_hal_trigger -L packets_received=950000
epon_hal_trigger -L bytes_sent=1024000000
epon_hal_trigger -L bytes_received=972800000

sleep 2

echo "Querying TR-181 parameters via RBUS..."
rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsSent"
rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsReceived"
rbus-cli get "Device.Ethernet.Link.1.Stats.BytesSent"
rbus-cli get "Device.Ethernet.Link.1.Stats.BytesReceived"

echo "Simulating traffic..."
for i in {1..10}; do
    epon_hal_trigger -I packets_sent=10000
    epon_hal_trigger -I packets_received=9800
    epon_hal_trigger -I bytes_sent=15360000
    epon_hal_trigger -I bytes_received=15052800
    sleep 1
done

echo "Querying updated statistics..."
rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsSent"  # Should be ~1100000
rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsReceived"  # Should be ~1048000

echo "Testing optical transceiver stats..."
epon_hal_trigger -T tx_power=-3.5
epon_hal_trigger -T rx_power=-18.2
epon_hal_trigger -T temperature=52.3
sleep 1
rbus-cli get "Device.Optical.Interface.1.TransmitPower"
rbus-cli get "Device.Optical.Interface.1.ReceivePower"
rbus-cli get "Device.Optical.Interface.1.Temperature"
```

## Integration with EPON Manager

The HAL mock library integrates with EPON Manager's callback system via **Unix Domain Socket IPC**:

1. **During `epon_hal_init()`**:
   - HAL mock creates a control socket at `/tmp/epon_hal_mock.sock`
   - Spawns a background thread to listen for commands
   - Stores the registered callbacks (status, alarm, interface)

2. **When you run `epon_hal_trigger`**:
   - Tool connects to `/tmp/epon_hal_mock.sock`
   - Sends command in format: `TYPE:param1:param2`
   - HAL mock receives command and invokes appropriate callback
   - Callback executes in EPON Manager's process context

**Command Protocol:**
- `STATUS:<onu_status_value>` - Triggers status_callback()
- `ALARM:<alarm_type>:<1|0>` - Triggers alarm_callback() (1=raised, 0=cleared)
- `INTERFACE:<name>:<link_status>` - Triggers interface_status_callback()
- `LINKSTATS:<field>:<value>` - Updates link statistics field (absolute set)
- `TRANSCVRSTATS:<field>:<value>` - Updates transceiver statistics field
- `INCREMENT:<field>:<value>` - Increments link statistics field (relative change)

This allows you to test:
- TR-181 data model updates (via RBUS)
- WanManager PHY notifications
- RDK Logger output
- Telemetry event generation
- State machine transitions
- **Dynamic statistics updates and queries**
- **Transceiver optical parameter monitoring**

**Process Isolation:**
The trigger utility runs as a separate process from EPON Manager, ensuring:
- No shared memory corruption
- Clean process boundaries
- Can be run remotely via SSH
- Multiple triggers can connect simultaneously

## RDK Logger Integration

The HAL header now includes RDK Logger integration by default. HAL implementations will automatically use `LOG.RDK.EPONMANAGER` profile:

```c
// HAL logs will appear as:
// [LOG.RDK.EPONMANAGER] [function:line] message
```

To disable RDK logging and use a different backend, define `HAL_LOG_FUNCTION` before including `epon_hal.h`.

## Installation

On target device:
```
/usr/bin/epon_hal_trigger      # Trigger utility
/usr/lib/libepon_hal_mock.so*   # HAL mock library
```

## Notes

- The trigger utility requires EPON Manager to be running and initialized with the HAL mock library
- Events are triggered immediately; use shell scripts for complex scenarios
- All events are logged to RDK Logger with appropriate log levels
- The mock library maintains internal state for statistics and configuration
