# Sequence Diagrams

## 1. System Startup Sequence

```mermaid
sequenceDiagram
    participant Main
    participant Controller as EPON Controller
    participant Logger
    participant RBus as RBus Thread
    participant Telemetry
    participant HAL
    participant EventListener
    participant StatsPoller
    
    Main->>Controller: epon_controller_init()
    activate Controller
    
    Controller->>Logger: logger_init()
    activate Logger
    Logger-->>Controller: Success
    deactivate Logger
    
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
    
    Controller->>HAL: hal_init()
    activate HAL
    HAL->>HAL: init_hardware()
    HAL-->>Controller: Success
    deactivate HAL
    
    Controller->>EventListener: event_listener_start()
    activate EventListener
    EventListener->>EventListener: start_event_loop()
    EventListener-->>Controller: Started
    
    Controller->>StatsPoller: stats_poller_start()
    activate StatsPoller
    StatsPoller->>StatsPoller: start_polling_loop()
    StatsPoller-->>Controller: Started
    
    Controller-->>Main: Ready
    deactivate Controller
    
    Note over Controller,StatsPoller: System Running
```

---

## 2. Link UP Event Sequence

```mermaid
sequenceDiagram
    participant HAL as EPON HAL
    participant EventQ as Event Queue
    participant EventL as Event Listener
    participant Controller
    participant Cache
    participant RBus
    participant WanMgr as WanManager
    participant Telem as Telemetry
    
    HAL->>EventQ: push(ONU_STATUS_UP)
    activate EventQ
    EventQ-->>HAL: OK
    deactivate EventQ
    
    EventL->>EventQ: pop()
    activate EventL
    EventQ-->>EventL: ONU_STATUS_UP event
    
    EventL->>Controller: processEvent(ONU_STATUS_UP)
    activate Controller
    
    Controller->>HAL: epon_hal_get_interface_list()
    activate HAL
    HAL-->>Controller: interface_list[]
    deactivate HAL
    
    Controller->>Cache: update_interface_list(list)
    activate Cache
    Cache-->>Controller: OK
    deactivate Cache
    
    Controller->>RBus: publish_status_event(PHY_UP)
    activate RBus
    RBus->>WanMgr: update_virtual_interface_list(list)
    activate WanMgr
    WanMgr-->>RBus: ACK
    deactivate WanMgr
    RBus->>WanMgr: notify(EPON_PHY_STATUS_UP)
    activate WanMgr
    WanMgr-->>RBus: ACK
    deactivate WanMgr
    

    RBus-->>Controller: OK
    deactivate RBus
    
    Controller->>Telem: report_event(LINK_UP)
    activate Telem
    Telem-->>Controller: OK
    deactivate Telem
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
```

---

## 3. Link DOWN Event Sequence

```mermaid
sequenceDiagram
    participant HAL as EPON HAL
    participant EventL as Event Listener
    participant Controller
    participant RBus
    participant WanMgr as WanManager
    participant Logger
    
    HAL->>EventL: epon_onu_status(DOWN)
    activate EventL
    
    EventL->>Controller: processEvent(ONU_STATUS_DOWN)
    activate Controller
    
    Controller->>Logger: log(INFO, "Link DOWN detected")
    
    Controller->>RBus: publish_status_event(PHY_DOWN)
    activate RBus
    RBus->>WanMgr: notify(EPON_PHY_STATUS_DOWN)
    activate WanMgr
    WanMgr-->>RBus: ACK
    deactivate WanMgr
    RBus-->>Controller: OK
    deactivate RBus
    
    Controller-->>EventL: Processed
    deactivate Controller
    deactivate EventL
```

---

## 4. TR-181 GET Request with Cache

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

## 5. TR-181 SET Request

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

## 6. Stats Harvesting Sequence

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

## 7. Alarm Event Sequence

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

## 8. Error Recovery Sequence

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
