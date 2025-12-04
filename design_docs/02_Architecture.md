# System Architecture

## High-Level Architecture

### System Context

```mermaid
graph TB
    User[RDK Components/Users]
    WM[WanManager]
    T2[Telemetry System T2]
    
    subgraph RdkEponManager
        EPM[EPON Manager]
    end
    
    HAL[EPON HAL]
    HW[EPON ONU Hardware]
    
    User -->|TR-181 Get/Set| EPM
    EPM -->|PHY Status Updates| WM
    EPM -->|Statistics/Events| T2
    EPM <-->|HAL API Calls| HAL
    HAL <-->|Hardware Control| HW
    
    style EPM fill:#4A90E2,stroke:#2E5C8A,color:#fff
    style HAL fill:#50C878,stroke:#2E7D4E,color:#fff
    style HW fill:#FF6B6B,stroke:#C92A2A,color:#fff
```

## Component Architecture

### Block Diagram

```mermaid
graph TB
    subgraph RdkEponManager
        subgraph Support["Support Modules"]
            Logger[Logger<br/>Module]
            Telemetry[Telemetry<br/>Module]
            RBus[RBus/DBus<br/>Thread]
        end
        
        Controller[EPON Controller<br/>Main Thread]
        
        subgraph Workers["Worker Threads & Storage"]
            EventListener[HAL Event<br/>Listener Thread]
            StatsPoller[Stats Polling<br/>Thread]
            Cache[(Internal<br/>Memory Cache)]
            EventQ[Events<br/>Queue]
        end
        
        Support --- Controller
        Controller --- Workers
    end
    
    subgraph EPONHAL["EPON HAL"]
        StatsAPI[Stats<br/>API]
        InterfaceAPI[Interface<br/>List API]
        AlarmAPI[Alarms<br/>API]
    end
    
    HW[Hardware<br/>EPON ONU]
    WanMgr[WanManager<br/>EPON PHY Status<br/>Virtual Interface List]
    
    Controller <--> EPONHAL
    EPONHAL -->|Push Events| EventQ
    EventListener -->|Poll Events| EventQ
    StatsPoller --> StatsAPI
    EPONHAL <--> HW
    RBus -.->|Updates via RBus| WanMgr
    
    style RdkEponManager fill:#E3F2FD,stroke:#1976D2,stroke-width:3px
    style Support fill:#FFF3E0,stroke:#F57C00
    style Workers fill:#F3E5F5,stroke:#7B1FA2
    style EPONHAL fill:#E8F5E9,stroke:#388E3C
    style Controller fill:#4A90E2,stroke:#2E5C8A,color:#fff
    style HW fill:#FFEBEE,stroke:#C62828
    style Cache fill:#FFD700,stroke:#B8860B
```

## C4 Model Diagrams

### Level 1: System Context

```mermaid
graph TB
    subgraph External Systems
        User[RDK Components<br/>Web UI, CLI, Other Services]
        WanMgr[WanManager]
        T2[T2 Telemetry]
    end
    
    EPONMgr[EPON Manager<br/>Manages EPON Configuration<br/>and Status]
    
    subgraph Hardware Layer
        HAL[EPON HAL]
        ONU[EPON ONU<br/>Hardware]
    end
    
    User -->|TR-181 Get/Set<br/>via RBus| EPONMgr
    EPONMgr -->|Link Status<br/>Interface Updates| WanMgr
    EPONMgr -->|Metrics & Events| T2
    EPONMgr <-->|HAL API| HAL
    HAL <-->|Driver Calls| ONU
    
    style EPONMgr fill:#1168bd,stroke:#0b4884,color:#ffffff
    style HAL fill:#438dd5,stroke:#2e6295,color:#ffffff
    style ONU fill:#85bbf0,stroke:#5d8ab8,color:#000000
```

### Level 2: Container Diagram

```mermaid
graph TB
    subgraph RdkEponManager Process
        Controller[EPON Controller<br/>Main Thread<br/>C Application]
        RBusThread[RBus Thread<br/>TR-181 Handler<br/>C Thread]
        EventListener[Event Listener<br/>HAL Event Handler<br/>C Thread]
        StatsPoller[Stats Poller<br/>Harvester<br/>C Thread]
        Logger[Logger Module<br/>RDK Logger Wrapper<br/>C Library]
        Telemetry[Telemetry Module<br/>T2 Wrapper<br/>C Library]
        Cache[Memory Cache<br/>Stats Cache<br/>In-Memory]
    end
    
    User[RDK Components] -->|RBus Messages| RBusThread
    RBusThread -->|Get/Set Requests| Controller
    RBusThread -->|Read Cache| Cache
    
    Controller -->|Init/Control| EventListener
    Controller -->|Init/Control| StatsPoller
    Controller -->|HAL Calls| HAL[EPON HAL]
    
    EventListener -->|Receive Events| HAL
    EventListener -->|Update Status| RBusThread
    
    StatsPoller -->|Poll Stats| HAL
    StatsPoller -->|Update| Cache
    StatsPoller -->|Report| Telemetry
    
    Controller -->|Log| Logger
    EventListener -->|Log| Logger
    StatsPoller -->|Log| Logger
    RBusThread -->|Log| Logger
    
    RBusThread -->|Notify| WanMgr[WanManager]
    Telemetry -->|Send Events| T2[T2 System]
    
    style Controller fill:#1168bd,stroke:#0b4884,color:#ffffff
    style RBusThread fill:#438dd5,stroke:#2e6295,color:#ffffff
    style EventListener fill:#438dd5,stroke:#2e6295,color:#ffffff
    style StatsPoller fill:#438dd5,stroke:#2e6295,color:#ffffff
```

## Controller State Machine

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    Uninitialized --> Initializing: Start
    Initializing --> Ready: Init Success
    Initializing --> Error: Init Failed
    Ready --> Running: All Threads Started
    Running --> Ready: Pause
    Running --> Shutdown: Stop Request
    Error --> Uninitialized: Reset
    Shutdown --> [*]
```

## TR-181 Data Model

```
Device.Ethernet.Link.{i}.
 Enable
 Status
 Name
 MACAddress
 Stats.
    BytesSent
    BytesReceived
    PacketsSent
    PacketsReceived
    ErrorsSent
    ErrorsReceived

Device.EPON.
 ONU.{i}.
   Enable
   Status
   LLID
   MACAddress
 Link.{i}.
    Status
    InterfaceList
```

## System Layers

| Layer | Components | Responsibility |
|-------|-----------|----------------|
| **Application Layer** | RDK Components, WebUI, CLI | User interaction and high-level control |
| **Service Layer** | WanManager, Telemetry (T2) | System services and coordination |
| **Manager Layer** | RdkEponManager | EPON-specific management and orchestration |
| **Abstraction Layer** | EPON HAL | Hardware abstraction and driver interface |
| **Hardware Layer** | EPON ONU | Physical EPON hardware |

## Communication Patterns

### Bus Communication (RBus)
- **Pattern**: Request-Response, Publish-Subscribe
- **Protocol**: RBus/DBus
- **Usage**: TR-181 GET/SET, Event notifications

### Event Queue
- **Pattern**: Producer-Consumer
- **Implementation**: Thread-safe queue
- **Usage**: HAL events to Event Listener

### Shared Memory
- **Pattern**: Cache with TTL
- **Synchronization**: Mutex-protected
- **Usage**: Statistics caching

---

**Document Version:** 1.0  
**Last Updated:** December 2, 2025
