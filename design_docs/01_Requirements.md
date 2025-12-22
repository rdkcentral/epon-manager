# Requirements Specification

## Functional Requirements

### FR1: EPON Controller

**Priority:** High

The EPON Controller is the main component responsible for system initialization and coordination.

- **FR1.1**: Initialize communication bus (RBus/DBus)
- **FR1.2**: Initialize logger subsystem
- **FR1.3**: Initialize telemetry subsystem
- **FR1.4**: Initialize internal memory and caching
- **FR1.5**: Initialize and start Stats Polling thread
- **FR1.6**: Initialize EPON HAL library
  - Register ONU status callback for link state events
  - Register interface status callback for per-interface state tracking
  - Register Alarm event callback for alarm notifications
- **FR1.7**: Create and manage queue listener for HAL events
- **FR1.8**: Process HAL events and update other RDK components
- **FR1.9**: (Optional) Implement cache mechanism for Stats DML queries
  - Only query HAL if stats are more than 30 seconds old
  - Provide cached responses for recent queries

### FR2: Stats Polling Thread (Harvester)

**Priority:** Medium

An optional thread for periodic statistics collection.

- **FR2.1**: Start as optional thread from EPON controller
- **FR2.2**: Poll EPON HAL statistics at configurable intervals
  - Default interval: 15 minutes
  - Configurable via configuration file
- **FR2.3**: Update telemetry system with collected statistics
- **FR2.4**: Update internal cache with fresh statistics

### FR3: RBus/DBus Thread

**Priority:** High

Handles all bus communication and TR-181 DML operations.

- **FR3.1**: Initialize bus communication (RBus or DBus)
- **FR3.2**: Register all TR-181 DML parameters
- **FR3.3**: Register RBus events for system notifications
- **FR3.4**: Handle incoming TR-181 GET requests
- **FR3.5**: Handle incoming TR-181 SET requests
- **FR3.6**: Provide API to update WanManager with PHY status (interface status)
- **FR3.7**: Provide API to update WanManager with virtual interface table

### FR4: Telemetry Module

**Priority:** Medium

Integration with RDK telemetry system.

- **FR4.1**: Register with telemetry applications (T2)
- **FR4.2**: Provide API wrapper for telemetry libraries
- **FR4.3**: Report critical events to telemetry
- **FR4.4**: Report periodic statistics to telemetry
- **FR4.5**: Format telemetry data according to T2 specifications

### FR5: Logger Module

**Priority:** High

Comprehensive logging support for debugging and monitoring.

- **FR5.1**: Initialize RDK logger for EPON manager
- **FR5.2**: Provide EPON logger wrapper linked with RDK logger APIs
- **FR5.3**: Support multiple log levels (FATAL, ERROR, WARNING, INFO, DEBUG)
- **FR5.4**: (Optional) Provide logger wrapper for HAL libraries
- **FR5.5**: Enable runtime log level configuration

### FR6: Event Processing

**Priority:** High

Handle events from EPON HAL.

- **FR6.1**: Process `epon_onu_status` callback events (Internal State Tracking)
  - Simplified 4-state model: LOS, DOWNSTREAM_SIGNAL_DETECTED, REGISTRATION, DEREGISTRATION
  - **FR6.1.1**: On REGISTRATION:
    - Update internal EPON ONU status to UP
    - Log successful registration
    - Raise telemetry event
  - **FR6.1.2**: On DEREGISTRATION or LOS:
    - Update internal EPON ONU status to DOWN
    - Log deregistration/LOS event
    - Raise telemetry event
  - **FR6.1.3**: On DOWNSTREAM_SIGNAL_DETECTED:
    - Update internal EPON ONU status to Initializing
    - Log status transition
  - **Note**: ONU status events do NOT trigger WanManager notifications
- **FR6.2**: Process `interface_status` callback events (PRIMARY - WanManager Updates)
  - **FR6.2.1**: On Interface LINK_UP (veip0, veip1, etc.):
    - Update interface list with interface name if not present
    - Update interface status to UP in cache
    - **Notify WanManager of interface availability via RBus with interface name**
        - If any of the interfaces go UP, WanManager should be notified that EPON PHY is UP
    - Raise telemetry event
  - **FR6.2.2**: On Interface LINK_DOWN:
    - Update interface status to DOWN in cache
    - **Notify WanManager of interface down via RBus with interface name**
        - If all of the interfaces go DOWN, WanManager should be notified that EPON PHY is DOWN
    - Raise telemetry event
- **FR6.2**: Process `epon_hal_alarm` events
  - Log alarm with appropriate severity
  - Raise telemetry event for critical/error alarms
  - Store alarm history (optional)

## Non-Functional Requirements

### NFR1: Performance

- **NFR1.1**: Event processing latency < 100ms
- **NFR1.2**: Stats query response < 10ms (cached), < 200ms (HAL)
- **NFR1.3**: Cache hit rate > 90% for stats queries
- **NFR1.4**: Thread CPU overhead < 10%
- **NFR1.5**: Memory footprint < 20MB

### NFR2: Reliability

- **NFR2.1**: Thread-safe operations across all components
- **NFR2.2**: Graceful handling of HAL failures
- **NFR2.3**: Auto-reconnect on RBus disconnection
- **NFR2.4**: Watchdog integration for process monitoring
- **NFR2.5**: No memory leaks during continuous operation

### NFR3: Maintainability

- **NFR3.1**: Modular component design
- **NFR3.2**: Clear separation of concerns
- **NFR3.3**: Comprehensive logging for debugging
- **NFR3.4**: Configuration via external file
- **NFR3.5**: Support for runtime reconfiguration (where applicable)

### NFR4: Scalability

- **NFR4.1**: Support configurable polling intervals
- **NFR4.2**: Configurable cache size and TTL
- **NFR4.3**: Handle multiple interface instances
- **NFR4.4**: Support for future multi-ONU scenarios

### NFR5: Compatibility

- **NFR5.1**: RDK-B platform compatibility
- **NFR5.2**: TR-181 data model compliance
- **NFR5.3**: Standard EPON HAL interface
- **NFR5.4**: RBus and DBus support

## Constraints

- **C1**: Must use existing RDK logging framework
- **C2**: Must comply with TR-181 data model specifications
- **C3**: Must use POSIX threads for multi-threading
- **C4**: Must integrate with existing T2 telemetry
- **C5**: Must work with standard EPON HAL interface

## Assumptions

- **A1**: EPON HAL library is available and functional
- **A2**: RBus/DBus infrastructure is initialized before EPON Manager starts
- **A3**: WanManager is running and listening on RBus
- **A4**: T2 telemetry service is available
- **A5**: System has sufficient resources for multi-threaded operation

---

**Document Version:** 1.0  
**Last Updated:** December 2, 2025
