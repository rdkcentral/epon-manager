# Sequence Diagrams

## 1. System Startup Sequence

```mermaid
sequenceDiagram
    participant Main
    participant Controller as EPON Controller
    participant Logger
    participant EventListener
    participant HAL
    participant RBus as RBus Thread
    participant Telemetry
    participant StatsPoller
    
    Main->>Controller: epon_controller_init()
    activate Controller
    
    Controller->>Logger: logger_init()
    activate Logger
    Logger-->>Controller: Success
    deactivate Logger
    
    Controller->>EventListener: event_listener_start()
    activate EventListener
    EventListener->>EventListener: start_event_loop()
    EventListener-->>Controller: Started
    
    Controller->>HAL: hal_init(callbacks)
    activate HAL
    HAL->>HAL: init_hardware()
    HAL->>HAL: register_onu_status_callback()
    HAL->>HAL: register_interface_status_callback()
    HAL->>HAL: register_alarm_callback()
    HAL-->>Controller: Success
    deactivate HAL
    
    Controller->>RBus: rbus_thread_start()
    activate RBus
    RBus->>RBus: init_bus()
    RBus->>RBus: register_dml_params()
    RBus-->>Controller: Started
    
    Controller->>Telemetry: telemetry_init()
    activate Telemetry
    Telemetry->>Telemetry: register_with_t2()
    Telemetry-->>Controller: Success
    deactivate Telemetry
    
    Controller->>StatsPoller: stats_poller_start()
    activate StatsPoller
    StatsPoller->>StatsPoller: start_polling_loop()
    StatsPoller-->>Controller: Started
    
    Controller-->>Main: Ready
    deactivate Controller
    
    Note over Controller,StatsPoller: System Running
```

---

## 2. ONU Registration Event Sequence (Internal State Only)

```mermaid
sequenceDiagram
    participant HW as EPON ONU Hardware
    participant HAL as EPON HAL
    participant EventL as Event Listener
    participant Controller
    participant Cache
    participant Logger
    participant Telem as Telemetry
    
    HW->>HAL: ONU Registration Complete
    
    HAL->>EventL: onu_status_callback(REGISTRATION)
    activate EventL
    
    EventL->>Controller: processONUStatus(REGISTRATION)
    activate Controller
    
    Controller->>Cache: update_internal_onu_status(UP)
    activate Cache
    Cache-->>Controller: OK
    deactivate Cache
    
    Controller->>Logger: log(INFO, "ONU Registered")
    activate Logger
    Logger-->>Controller: OK
    deactivate Logger
    
    Controller->>Telem: report_event(ONU_REGISTERED)
    activate Telem
    Telem-->>Controller: OK
    deactivate Telem
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
    
    Note over HW,Telem: No WanManager update - waiting for interface callbacks
```

---

## 3. Interface Link UP Event Sequence

```mermaid
sequenceDiagram
    participant HW as EPON ONU Hardware
    participant HAL as EPON HAL
    participant EventL as Event Listener
    participant Controller
    participant Cache
    participant RBus
    participant WanMgr as WanManager
    participant Telem as Telemetry
    
    HW->>HAL: Interface veip0 Link UP
    
    HAL->>EventL: interface_status_callback(veip0, LINK_UP)
    activate EventL
    
    EventL->>Controller: processInterfaceStatus(veip0, LINK_UP)
    activate Controller
    
    Controller->>Cache: update_interface_status(veip0, UP)
    activate Cache
    Cache->>Cache: add_to_interface_list(veip0)
    Cache-->>Controller: OK
    deactivate Cache
    
    Controller->>Controller: Check if first interface UP
    
    Controller->>RBus: publish_interface_event(veip0, UP)
    activate RBus
    
    alt First Interface Coming UP
        RBus->>WanMgr: notify(EPON_PHY_STATUS_UP)
        activate WanMgr
        WanMgr-->>RBus: ACK
        deactivate WanMgr
        Note over RBus,WanMgr: PHY goes UP when any interface is UP
    end
    
    RBus->>WanMgr: update_virtual_interface(veip0, AVAILABLE)
    activate WanMgr
    WanMgr-->>RBus: ACK
    deactivate WanMgr
    RBus-->>Controller: OK
    deactivate RBus
    
    Controller->>Telem: report_event(INTERFACE_UP, veip0)
    activate Telem
    Telem-->>Controller: OK
    deactivate Telem
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
```

---

## 4. Interface Link DOWN Event Sequence

```mermaid
sequenceDiagram
    participant HW as EPON ONU Hardware
    participant HAL as EPON HAL
    participant EventL as Event Listener
    participant Controller
    participant Cache
    participant RBus
    participant WanMgr as WanManager
    participant Telem as Telemetry
    participant Logger
    
    HW->>HAL: Interface veip0 Link DOWN
    
    HAL->>EventL: interface_status_callback(veip0, LINK_DOWN)
    activate EventL
    
    EventL->>Controller: processInterfaceStatus(veip0, LINK_DOWN)
    activate Controller
    
    Controller->>Logger: log(WARNING, "Interface veip0 DOWN")
    activate Logger
    Logger-->>Controller: OK
    deactivate Logger
    
    Controller->>Cache: update_interface_status(veip0, DOWN)
    activate Cache
    Cache->>Cache: remove_from_interface_list(veip0)
    Cache-->>Controller: OK
    deactivate Cache
    
    Controller->>Controller: Check if all interfaces DOWN
    
    Controller->>RBus: publish_interface_event(veip0, DOWN)
    activate RBus
    RBus->>WanMgr: update_virtual_interface(veip0, UNAVAILABLE)
    activate WanMgr
    WanMgr-->>RBus: ACK
    deactivate WanMgr
    
    alt All Interfaces Now DOWN
        RBus->>WanMgr: notify(EPON_PHY_STATUS_DOWN)
        activate WanMgr
        WanMgr-->>RBus: ACK
        deactivate WanMgr
        Note over RBus,WanMgr: PHY goes DOWN only when all interfaces are DOWN
    end
    
    RBus-->>Controller: OK
    deactivate RBus
    
    Controller->>Telem: report_event(INTERFACE_DOWN, veip0)
    activate Telem
    Telem-->>Controller: OK
    deactivate Telem
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
```

---

## 5. ONU Deregistration Event Sequence (Internal State Only)

```mermaid
sequenceDiagram
    participant HW as EPON ONU Hardware
    participant HAL as EPON HAL
    participant EventL as Event Listener
    participant Controller
    participant Cache
    participant Logger
    participant Telem as Telemetry
    
    HW->>HAL: ONU Deregistration / LOS
    
    HAL->>EventL: onu_status_callback(DEREGISTRATION)
    activate EventL
    
    EventL->>Controller: processONUStatus(DEREGISTRATION)
    activate Controller
    
    Controller->>Logger: log(WARNING, "ONU Deregistered")
    activate Logger
    Logger-->>Controller: OK
    deactivate Logger
    
    Controller->>Cache: update_internal_onu_status(DOWN)
    activate Cache
    Cache-->>Controller: OK
    deactivate Cache
    
    Controller->>Telem: report_event(ONU_DEREGISTERED)
    activate Telem
    Telem-->>Controller: OK
    deactivate Telem
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
    
    Note over HW,Telem: No WanManager update - interface callbacks handle that
```

---

## 6. TR-181 GET Request with Cache

```mermaid
sequenceDiagram
    participant Client as RDK Component
    participant RBus as RBus Thread
    participant Cache
    participant Controller
    participant HAL
    
    Client->>RBus: GET Stats.BytesSent
    activate RBus
    
    RBus->>Cache: cache_get("Stats.BytesSent")
    activate Cache
    
    alt Cache Valid (<30s)
        Cache-->>RBus: value, valid=true
        RBus-->>Client: Return cached value
    else Cache Invalid (>30s)
        Cache-->>RBus: valid=false
        deactivate Cache
        
        RBus->>Controller: query_hal_stats()
        activate Controller
        
        Controller->>HAL: epon_hal_get_stats()
        activate HAL
        HAL-->>Controller: stats_data
        deactivate HAL
        
        Controller->>Cache: cache_set("Stats.BytesSent", value, 30)
        activate Cache
        Cache-->>Controller: OK
        deactivate Cache
        
        Controller-->>RBus: stats_data
        deactivate Controller
        
        RBus-->>Client: Return fresh value
    end
    
    deactivate RBus
```

---

## 7. TR-181 SET Request

```mermaid
sequenceDiagram
    participant Client as RDK Component
    participant RBus as RBus Thread
    participant Controller
    participant HAL
    participant Cache
    participant Logger
    
    Client->>RBus: SET Device.EPON.ONU.1.Enable=true
    activate RBus
    
    RBus->>Controller: set_parameter(Enable, true)
    activate Controller
    
    Controller->>Controller: validate_parameter()
    
    alt Valid Parameter
        Controller->>HAL: epon_hal_set_enable(true)
        activate HAL
        HAL-->>Controller: Success
        deactivate HAL
        
        Controller->>Cache: invalidate_related_cache()
        activate Cache
        Cache-->>Controller: OK
        deactivate Cache
        
        Controller->>Logger: log(INFO, "Parameter set")
        
        Controller->>RBus: publish_event(PARAM_CHANGED)
        
        Controller-->>RBus: Success
        RBus-->>Client: Success
    else Invalid Parameter
        Controller->>Logger: log(ERROR, "Invalid parameter")
        Controller-->>RBus: Error
        RBus-->>Client: Error
    end
    
    deactivate Controller
    deactivate RBus
```

---

## 8. Stats Harvesting Sequence

```mermaid
sequenceDiagram
    participant Timer
    participant StatsPoller
    participant HAL
    participant Cache
    participant Telem as Telemetry
    participant Logger
    
    loop Every 15 minutes
        Timer->>StatsPoller: Timer Expired
        activate StatsPoller
        
        StatsPoller->>Logger: log(INFO, "Harvesting stats")
        
        StatsPoller->>HAL: epon_hal_get_all_stats()
        activate HAL
        HAL-->>StatsPoller: all_stats
        deactivate HAL
        
        loop For each stat parameter
            StatsPoller->>Cache: cache_set(param, value, 30)
            activate Cache
            Cache-->>StatsPoller: OK
            deactivate Cache
        end
        
        StatsPoller->>Telem: report_stats(all_stats)
        activate Telem
        Telem->>Telem: format_for_t2()
        Telem->>Telem: send_to_t2()
        Telem-->>StatsPoller: OK
        deactivate Telem
        
        StatsPoller->>Logger: log(INFO, "Harvest complete")
        
        deactivate StatsPoller
    end
```

---

## 9. Alarm Event Sequence

```mermaid
sequenceDiagram
    participant HAL
    participant EventL as Event Listener
    participant Controller
    participant Logger
    participant Telem as Telemetry
    
    HAL->>EventL: alarm_event(severity, id, desc)
    activate EventL
    
    EventL->>Controller: processAlarm(alarm)
    activate Controller
    
    Controller->>Logger: log(severity, alarm.desc)
    activate Logger
    Logger->>Logger: write_to_log_file()
    Logger-->>Controller: OK
    deactivate Logger
    
    alt Severity is Critical or Error
        Controller->>Telem: raise_telemetry_event(alarm)
        activate Telem
        Telem->>Telem: format_alarm_event()
        Telem->>Telem: send_to_t2()
        Telem-->>Controller: OK
        deactivate Telem
    else Severity is Warning or Info
        Controller->>Controller: Log only (already done)
    end
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
```

---

## 10. Error Recovery Sequence

```mermaid
sequenceDiagram
    participant Component
    participant HAL
    participant Logger
    participant Controller
    
    Component->>HAL: hal_operation()
    activate HAL
    HAL-->>Component: Error
    deactivate HAL
    
    Component->>Logger: log(ERROR, "HAL failure")
    
    Component->>Component: attempt = 1
    
    loop Retry up to 3 times
        Component->>Component: wait(backoff_delay)
        Component->>HAL: hal_operation()
        activate HAL
        
        alt Success
            HAL-->>Component: Success
            Component->>Logger: log(INFO, "Recovery successful")
        else Still Failing
            HAL-->>Component: Error
            Component->>Component: attempt++
        end
        deactivate HAL
    end
    
    alt Max Retries Exceeded
        Component->>Logger: log(FATAL, "HAL unrecoverable")
        Component->>Controller: notify_critical_error()
        activate Controller
        Controller->>Controller: trigger_watchdog_restart()
        deactivate Controller
    end
```

---

**Document Version:** 1.0  
**Last Updated:** December 2, 2025
