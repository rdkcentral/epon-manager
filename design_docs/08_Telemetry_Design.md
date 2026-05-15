# EPON Manager — Telemetry Module Design

**Version:** 1.1  |  **Date:** May 15, 2026
**Companion:** [07_Telemetry_Implementation_Plan.md](07_Telemetry_Implementation_Plan.md)
**Spec:** [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) §2

---

## 1. Design goals

1. **Single producer surface** — the rest of the code base only knows
   *event ids*. Marker names, severity, value formatting and T2 dispatch
   are all hidden inside `src/telemetry/`.
2. **Replaceable backend** — T2 stub vs. real `libtelemetry_msgsender` is
   guarded at link/compile time only. Caller code never changes.

---

## 2. Module boundary

```mermaid
flowchart LR
    subgraph caller [Rest of EPON Manager]
        MAIN[epon_manager_main.c]
        CTRL[controller.c]
        SP[stats_poller.c]
        RB[rbus / tr181 / psm]
    end

    subgraph telem [src/telemetry/  — sole owner of telemetry logic]
        API[[eponMgr_telemetry.h<br/>3 producer APIs]]
        TELEM(eponMgr_telemetry.c<br/>table + format + alarm-map<br/>+ T2 backend + dispatcher)
    end

    subgraph ext [External]
        T2[T2 daemon<br/>libtelemetry_msgsender]
    end

    MAIN --> API
    CTRL --> API
    SP   --> API
    RB   --> API

    API --> TELEM
    TELEM --> T2
```

> Only `eponMgr_telemetry.h` crosses the module boundary.

---

## 3. File responsibilities

The telemetry module is implemented in **1 `.c` file** — a single
self-contained module.

| File | Owns |
|------|------|
| `eponMgr_telemetry.h` | Public surface: 34 event ids, 3 producer APIs (`_raise_simple`, `_raise_intf`, `_raise_alarm`), lifecycle APIs. Only header callers ever include. |
| `eponMgr_telemetry.c` | Self-contained event module. Five sections inside the file: (1) const event descriptor table, (2) value formatter (`Interface=…`, `RAISED`/`CLEARED[,LLID=N]`), (3) HAL alarm → event-id mapper, (4) T2 backend (`t2_event_s` shim, log-only when `HAVE_LIBT2` undefined), (5) dispatcher + public init/cleanup/raise. All internals are file-static. |

---

## 4. Event raise — sequence

```mermaid
sequenceDiagram
    autonumber
    participant Ctrl as controller.c
    participant API as eponMgr_telemetry.h
    participant Telem as eponMgr_telemetry.c
    participant T2D as T2 daemon

    Ctrl->>API: raise_intf(EPON_TELEM_INTF_LINK_UP, "veip0")
    API->>Telem: _raise(id, &ctx{ifname="veip0"})
    Note over Telem: lookup(id) -> { "EPON_INTF_LINK_UP",<br/>INFO, FMT_INTF }<br/>format -> "Interface=veip0"
    Telem->>T2D: t2_event_s("EPON_INTF_LINK_UP",<br/>"Interface=veip0")
    Telem-->>API: 0
    API-->>Ctrl: 0
```

---

## 5. Alarm raise — sequence (RAISED / CLEARED)

```mermaid
sequenceDiagram
    autonumber
    participant Ctrl as controller.c
    participant API as eponMgr_telemetry.h
    participant Telem as eponMgr_telemetry.c
    participant T2 as T2 daemon

    Note over Ctrl: HAL fired epon_alarm_info_t<br/>(STANDARD, LOFI, is_active=true,<br/>llid=0xFFFF)
    Ctrl->>API: raise_alarm(&info)
    API->>Telem: _raise_alarm(info)
    Note over Telem: map_alarm(info) ->
    Note over Telem: EPON_TELEM_ALARM_STD_LOFI<br/>format -> "RAISED"
    Telem->>T2: t2_event_s("EPON_ALARM_STD_LOFI",<br/>"RAISED")
```

When the alarm clears, the same path runs with `info->is_active=false`
→ value `"CLEARED"`.

---

## 6. Threading model

```mermaid
flowchart TB
    subgraph proc [rdk-eponmanager process]
        MT(main thread)
        EVT(event-processor thread<br/>controller.c)
        STP(stats-poller thread<br/>stats_poller.c)
    end

    EVT -- raise_simple/intf/alarm --> TELE([eponMgr_telemetry.c<br/>mutex-protected state])
    STP -- raise_simple --> TELE
    MT  -- raise_simple --> TELE

    TELE -- t2_event_s --> T2(T2 daemon)
```

* Producer side is fully thread-safe — internal mutex in `eponMgr_telemetry.c`.
* Multiple threads may call `_raise*` concurrently; the internal mutex
  serializes state updates (event counts) but `t2_event_s()` itself is
  called outside the critical section.

---

## 7. State machine — alarm raised/cleared encoding

```mermaid
stateDiagram-v2
    [*] --> Cleared
    Cleared --> Raised: HAL alarm_callback(is_active=true)<br/>raise_alarm(&info)<br/>value="RAISED"
    Raised  --> Cleared: HAL alarm_callback(is_active=false)<br/>raise_alarm(&info)<br/>value="CLEARED"
```

The state itself is owned by HAL; the telemetry module is **stateless**
with respect to alarms — it forwards each transition as one T2 event.

---

## 8. Error & resource handling

| Failure | Behaviour |
|---------|-----------|
| `_raise*` called before `_init` | Return `-1`; no log spam (rate-limited). |
| `_raise*` with unknown id | Return `-1`; log once. |
| T2 backend send error | Logged at `WARN`; producer return value unchanged (success), so callers don't cascade-fail on telemetry hiccups. |

---

## 9. Test plan

```mermaid
flowchart LR
    HAL_MOCK[tests/hal_mock<br/>epon_hal_trigger.c] --> CB1(status_callback)
    HAL_MOCK --> CB2(alarm_callback)
    HAL_MOCK --> CB3(interface_status_callback)
    CB1 --> CTRL2[controller.c]
    CB2 --> CTRL2
    CB3 --> CTRL2
    CTRL2 --> TELE2[telemetry::raise_*]
    TELE2 --> T2STUB[T2 stub log]

    T2STUB -.assert.-> ASSERT[34 ids fired]
```

Test driver iterates the trigger harness through every code path and
verifies all 34 event ids are observed.

---

## 10. Reference

* [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) — TR-181, telemetry spec.
* IEEE 802.3ah Clause 57 (OAM) — alarm taxonomy.
* RDK T2 Telemetry Framework — marker semantics.
