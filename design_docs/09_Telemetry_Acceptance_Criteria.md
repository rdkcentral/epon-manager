# EPON Manager — Telemetry Acceptance Criteria

**Version:** 1.1  |  **Date:** May 15, 2026
**Jira Story:** EPON Manager — Telemetry Implementation
**Format:** MoSCoW (Must / Should / Could / Won't)
**Source-of-truth spec:** [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) §2
**Companion:** [07_Telemetry_Design.md](07_Telemetry_Design.md)

---

## Conventions

* "**CPE**" = the gateway running `rdk-eponmanager`.
* "Telemetry event" = a discrete T2 marker dispatched via
  `t2_event_s(marker, value)`.
* "Within X seconds" is wall-clock from the originating HAL callback
  to the T2 send call.

---

## MUST (P0 — release blockers)

1. EPON Manager MUST send T2 events to monitor ONU status changes.
   - **1.1.** EPON Manager MUST send a T2 event when the HAL reports loss of downstream optical signal.
     - **1.1.1.** Event Name: `EPON_ONU_LOS`
     - **1.1.2.** Event Arguments: None
     - **1.1.3.** Example: `t2_event_s("EPON_ONU_LOS", "")`
     - **1.1.4.** Priority: Critical
     - **1.1.5.** Timing: MUST be raised within 1 second of the HAL `status_callback`.
     - **1.1.6.** Rate Limit: No rate limit; state-change driven (HAL callback).
   - **1.2.** EPON Manager MUST send a T2 event when downstream signal is detected but the ONU is not yet registered.
     - **1.2.1.** Event Name: `EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED`
     - **1.2.2.** Event Arguments: None
     - **1.2.3.** Example: `t2_event_s("EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED", "")`
     - **1.2.4.** Priority: Info
     - **1.2.5.** Rate Limit: No rate limit; state-change driven (HAL callback).
   - **1.3.** EPON Manager MUST send a T2 event when the ONU completes MPCP discovery and OAM negotiation.
     - **1.3.1.** Event Name: `EPON_ONU_REGISTRATION`
     - **1.3.2.** Event Arguments: None
     - **1.3.3.** Example: `t2_event_s("EPON_ONU_REGISTRATION", "")`
     - **1.3.4.** Priority: Info
     - **1.3.5.** Rate Limit: No rate limit; state-change driven (HAL callback).
   - **1.4.** EPON Manager MUST send a T2 event when the OLT deregisters the ONU or MPCP times out.
     - **1.4.1.** Event Name: `EPON_ONU_DEREGISTRATION`
     - **1.4.2.** Event Arguments: None
     - **1.4.3.** Example: `t2_event_s("EPON_ONU_DEREGISTRATION", "")`
     - **1.4.4.** Priority: Warning
     - **1.4.5.** Rate Limit: No rate limit; state-change driven (HAL callback).

2. EPON Manager MUST send T2 events to monitor EPON WAN interface link status.
   - **2.1.** EPON Manager MUST send a T2 event when an EPON WAN interface comes up.
     - **2.1.1.** Event Name: `EPON_INTF_LINK_UP`
     - **2.1.2.** Event Arguments(INTERFACE): `Interface=<ifname>` (e.g. `Interface=veip0`)
     - **2.1.3.** Example: `t2_event_s("EPON_INTF_LINK_UP", "Interface=veip0")`
     - **2.1.4.** Priority: Info
     - **2.1.5.** Rate Limit: No rate limit; state-change driven (interface link event).
   - **2.2.** EPON Manager MUST send a T2 event when an EPON WAN interface goes down.
     - **2.2.1.** Event Name: `EPON_INTF_LINK_DOWN`
     - **2.2.2.** Event Arguments(INTERFACE): `Interface=<ifname>` (e.g. `Interface=veip0`)
     - **2.2.3.** Example: `t2_event_s("EPON_INTF_LINK_DOWN", "Interface=veip0")`
     - **2.2.4.** Priority: Warning
     - **2.2.5.** Rate Limit: No rate limit; state-change driven (interface link event).
   - **2.3.** EPON Manager MUST send a T2 event when the first interface transitions UP after all were DOWN.
     - **2.3.1.** Event Name: `EPON_PHY_STATUS_UP`
     - **2.3.2.** Event Arguments: None
     - **2.3.3.** Example: `t2_event_s("EPON_PHY_STATUS_UP", "")`
     - **2.3.4.** Priority: Info
     - **2.3.5.** Rate Limit: No rate limit; state-change driven (aggregate PHY transition).
   - **2.4.** EPON Manager MUST send a T2 event when the last interface transitions DOWN.
     - **2.4.1.** Event Name: `EPON_PHY_STATUS_DOWN`
     - **2.4.2.** Event Arguments: None
     - **2.4.3.** Example: `t2_event_s("EPON_PHY_STATUS_DOWN", "")`
     - **2.4.4.** Priority: Critical
     - **2.4.5.** Rate Limit: No rate limit; state-change driven (aggregate PHY transition).

3. EPON Manager MUST send T2 events for Standard IEEE 802.3ah alarms on both RAISED and CLEARED transitions. When the alarm is per-LLID, the value MUST include `,LLID=<n>`.
   - **3.1.** EPON Manager MUST send a T2 event for Loss-of-Frame/Lock.
     - **3.1.1.** Event Name: `EPON_ALARM_STD_LOFI`
     - **3.1.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.1.3.** Example: `t2_event_s("EPON_ALARM_STD_LOFI", "RAISED,LLID=1")`
     - **3.1.4.** Priority: Critical
     - **3.1.5.** Rate Limit: No rate limit; state-change driven (alarm raised/cleared transition).
   - **3.2.** EPON Manager MUST send a T2 event when the Errored Symbol Period threshold is breached (IEEE 802.3ah Clause 57.5.2).
     - **3.2.1.** Event Name: `EPON_ALARM_STD_ERROR_SYMBOL_PERIOD`
     - **3.2.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.2.3.** Example: `t2_event_s("EPON_ALARM_STD_ERROR_SYMBOL_PERIOD", "RAISED,LLID=1")`
     - **3.2.4.** Priority: Error
     - **3.2.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **3.3.** EPON Manager MUST send a T2 event when the Errored Frame threshold is breached (Clause 57.5.3).
     - **3.3.1.** Event Name: `EPON_ALARM_STD_ERROR_FRAME`
     - **3.3.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.3.3.** Example: `t2_event_s("EPON_ALARM_STD_ERROR_FRAME", "RAISED,LLID=1")`
     - **3.3.4.** Priority: Warning
     - **3.3.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **3.4.** EPON Manager MUST send a T2 event when the Errored Frame Period threshold is breached (Clause 57.5.4).
     - **3.4.1.** Event Name: `EPON_ALARM_STD_ERROR_FRAME_PERIOD`
     - **3.4.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.4.3.** Example: `t2_event_s("EPON_ALARM_STD_ERROR_FRAME_PERIOD", "RAISED,LLID=1")`
     - **3.4.4.** Priority: Warning
     - **3.4.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **3.5.** EPON Manager MUST send a T2 event when the Errored Frame Seconds threshold is breached (Clause 57.5.5).
     - **3.5.1.** Event Name: `EPON_ALARM_STD_ERROR_FRAME_SECONDS`
     - **3.5.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.5.3.** Example: `t2_event_s("EPON_ALARM_STD_ERROR_FRAME_SECONDS", "RAISED,LLID=1")`
     - **3.5.4.** Priority: Warning
     - **3.5.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **3.6.** EPON Manager MUST send a T2 event when OAM keepalive detects loss of the OAM peer.
     - **3.6.1.** Event Name: `EPON_ALARM_STD_OAM_SESSION_LOST`
     - **3.6.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.6.3.** Example: `t2_event_s("EPON_ALARM_STD_OAM_SESSION_LOST", "RAISED,LLID=1")`
     - **3.6.4.** Priority: Critical
     - **3.6.5.** Rate Limit: No rate limit; state-change driven (OAM session event).
   - **3.7.** EPON Manager MUST send a T2 event when the HAL signals an ONU hardware failure.
     - **3.7.1.** Event Name: `EPON_ALARM_STD_EQUIPMENT_FAILURE`
     - **3.7.2.** Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
     - **3.7.3.** Example: `t2_event_s("EPON_ALARM_STD_EQUIPMENT_FAILURE", "RAISED,LLID=1")`
     - **3.7.4.** Priority: Critical
     - **3.7.5.** Rate Limit: No rate limit; state-change driven (HAL hardware fault).

4. EPON Manager MUST send T2 events for Vendor-Specific (DPoE) alarms on both RAISED and CLEARED transitions. These are physical/hardware-layer events scoped to the entire ONU transceiver; they carry no LLID because the HAL always returns `LLID=0xFFFF` (not applicable) for hardware-level conditions.
   - **4.1.** EPON Manager MUST send a T2 event on complete loss of optical signal at the receiver.
     - **4.1.1.** Event Name: `EPON_ALARM_VENDOR_LOS`
     - **4.1.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.1.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_LOS", "RAISED")`
     - **4.1.4.** Priority: Critical
     - **4.1.5.** Rate Limit: No rate limit; state-change driven (optical loss event).
   - **4.2.** EPON Manager MUST send a T2 event on imminent power loss before the ONU reboots.
     - **4.2.1.** Event Name: `EPON_ALARM_VENDOR_DYING_GASP`
     - **4.2.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.2.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_DYING_GASP", "RAISED")`
     - **4.2.4.** Priority: Critical
     - **4.2.5.** Rate Limit: No rate limit; fired once per power-loss event.
   - **4.3.** EPON Manager MUST send a T2 event when RX optical power drops below the lower threshold.
     - **4.3.1.** Event Name: `EPON_ALARM_VENDOR_POWER_LOW`
     - **4.3.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.3.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_POWER_LOW", "RAISED")`
     - **4.3.4.** Priority: Warning
     - **4.3.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **4.4.** EPON Manager MUST send a T2 event when RX optical power exceeds the upper threshold.
     - **4.4.1.** Event Name: `EPON_ALARM_VENDOR_POWER_HIGH`
     - **4.4.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.4.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_POWER_HIGH", "RAISED")`
     - **4.4.4.** Priority: Warning
     - **4.4.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **4.5.** EPON Manager MUST send a T2 event when the transceiver temperature exceeds the safe threshold.
     - **4.5.1.** Event Name: `EPON_ALARM_VENDOR_TEMPERATURE`
     - **4.5.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.5.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_TEMPERATURE", "RAISED")`
     - **4.5.4.** Priority: Error
     - **4.5.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **4.6.** EPON Manager MUST send a T2 event when uncorrectable FEC errors exceed the threshold.
     - **4.6.1.** Event Name: `EPON_ALARM_VENDOR_FEC_THRESHOLD`
     - **4.6.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.6.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_FEC_THRESHOLD", "RAISED")`
     - **4.6.4.** Priority: Error
     - **4.6.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **4.7.** EPON Manager MUST send a T2 event when laser bias current is out of range.
     - **4.7.1.** Event Name: `EPON_ALARM_VENDOR_LASER_BIAS_CURRENT`
     - **4.7.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.7.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_LASER_BIAS_CURRENT", "RAISED")`
     - **4.7.4.** Priority: Error
     - **4.7.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).
   - **4.8.** EPON Manager MUST send a T2 event when the transceiver supply voltage is out of nominal range.
     - **4.8.1.** Event Name: `EPON_ALARM_VENDOR_SUPPLY_VOLTAGE`
     - **4.8.2.** Event Arguments(STATE): `RAISED`, `CLEARED`
     - **4.8.3.** Example: `t2_event_s("EPON_ALARM_VENDOR_SUPPLY_VOLTAGE", "RAISED")`
     - **4.8.4.** Priority: Error
     - **4.8.5.** Rate Limit: No rate limit; state-change driven (threshold crossing).

5. EPON Manager MUST send T2 events for system lifecycle transitions.
   - **5.1.** EPON Manager MUST send a T2 event once after all subsystems (RBus, PSM, HAL, stats poller, telemetry) start successfully.
     - **5.1.1.** Event Name: `EPON_SYSTEM_INIT_SUCCESS`
     - **5.1.2.** Event Arguments: None
     - **5.1.3.** Example: `t2_event_s("EPON_SYSTEM_INIT_SUCCESS", "")`
     - **5.1.4.** Priority: Info
     - **5.1.5.** Rate Limit: Fired once per process start; structurally non-repeatable.
   - **5.2.** EPON Manager MUST send a T2 event if any subsystem fails to initialize.
     - **5.2.1.** Event Name: `EPON_SYSTEM_INIT_FAILURE`
     - **5.2.2.** Event Arguments: None
     - **5.2.3.** Example: `t2_event_s("EPON_SYSTEM_INIT_FAILURE", "")`
     - **5.2.4.** Priority: Critical
     - **5.2.5.** Rate Limit: Fired once per process start; structurally non-repeatable.
   - **5.3.** EPON Manager MUST send a T2 event on receipt of SIGTERM/SIGINT before tearing down.
     - **5.3.1.** Event Name: `EPON_SYSTEM_SHUTDOWN`
     - **5.3.2.** Event Arguments: None
     - **5.3.3.** Example: `t2_event_s("EPON_SYSTEM_SHUTDOWN", "")`
     - **5.3.4.** Priority: Info
     - **5.3.5.** Rate Limit: Fired once per shutdown signal; structurally non-repeatable.
   - **5.4.** EPON Manager MUST send a T2 event when the HAL reports hardware configured for a non-EPON mode.
     - **5.4.1.** Event Name: `EPON_SYSTEM_HAL_WRONG_PON_MODE`
     - **5.4.2.** Event Arguments: None
     - **5.4.3.** Example: `t2_event_s("EPON_SYSTEM_HAL_WRONG_PON_MODE", "")`
     - **5.4.4.** Priority: Critical
     - **5.4.5.** Rate Limit: No rate limit; state-change driven (HAL callback).
   - **5.5.** EPON Manager MUST send a T2 event when a factory reset is triggered via the TR-181 `FactoryReset` action.
     - **5.5.1.** Event Name: `EPON_SYSTEM_FACTORY_RESET`
     - **5.5.2.** Event Arguments: None
     - **5.5.3.** Example: `t2_event_s("EPON_SYSTEM_FACTORY_RESET", "")`
     - **5.5.4.** Priority: Warning
     - **5.5.5.** Rate Limit: No rate limit; operator-initiated action.
   - **5.6.** EPON Manager MUST send a T2 event when the TR-181 `Reset` action is invoked.
     - **5.6.1.** Event Name: `EPON_SYSTEM_ONU_RESET`
     - **5.6.2.** Event Arguments: None
     - **5.6.3.** Example: `t2_event_s("EPON_SYSTEM_ONU_RESET", "")`
     - **5.6.4.** Priority: Info
     - **5.6.5.** Rate Limit: No rate limit; operator-initiated action.

6. EPON Manager MUST send T2 events for runtime errors.
   - **6.1.** EPON Manager MUST send a T2 event when any HAL API returns a non-success status.
     - **6.1.1.** Event Name: `EPON_ERROR_HAL_CALL_FAILED`
     - **6.1.2.** Event Arguments: None
     - **6.1.3.** Example: `t2_event_s("EPON_ERROR_HAL_CALL_FAILED", "")`
     - **6.1.4.** Priority: Error
     - **6.1.5.** Rate Limit: Max 1 event per second per marker; prevents flooding on sustained HAL failures.
   - **6.2.** EPON Manager MUST send a T2 event when the internal event queue drops an event.
     - **6.2.1.** Event Name: `EPON_ERROR_EVENT_QUEUE_FULL`
     - **6.2.2.** Event Arguments: None
     - **6.2.3.** Example: `t2_event_s("EPON_ERROR_EVENT_QUEUE_FULL", "")`
     - **6.2.4.** Priority: Error
     - **6.2.5.** Rate Limit: Max 1 event per second per marker.
   - **6.3.** EPON Manager MUST send a T2 event when periodic stats retrieval from the HAL fails.
     - **6.3.1.** Event Name: `EPON_ERROR_STATS_COLLECTION_FAILED`
     - **6.3.2.** Event Arguments: None
     - **6.3.3.** Example: `t2_event_s("EPON_ERROR_STATS_COLLECTION_FAILED", "")`
     - **6.3.4.** Priority: Warning
     - **6.3.5.** Rate Limit: Max 1 event per `StatsPoller.PollingInterval`; naturally bounded by polling cadence.
   - **6.4.** EPON Manager MUST send a T2 event when an RBus publish or set fails.
     - **6.4.1.** Event Name: `EPON_ERROR_RBUS_PUBLISH_FAILED`
     - **6.4.2.** Event Arguments: None
     - **6.4.3.** Example: `t2_event_s("EPON_ERROR_RBUS_PUBLISH_FAILED", "")`
     - **6.4.4.** Priority: Error
     - **6.4.5.** Rate Limit: Max 1 event per second per marker.
   - **6.5.** EPON Manager MUST send a T2 event on PSM read/write failure.
     - **6.5.1.** Event Name: `EPON_ERROR_PSM_ACCESS_FAILED`
     - **6.5.2.** Event Arguments: None
     - **6.5.3.** Example: `t2_event_s("EPON_ERROR_PSM_ACCESS_FAILED", "")`
     - **6.5.4.** Priority: Error
     - **6.5.5.** Rate Limit: Max 1 event per second per marker.

7. EPON Manager MUST enforce a single-producer surface for all telemetry output.
   - **7.1.** All telemetry calls MUST go through one of the three public APIs declared in [`include/eponMgr_telemetry.h`](../include/eponMgr_telemetry.h).
     - **7.1.1.** API: `eponMgr_telemetry_raise_simple`
     - **7.1.2.** API: `eponMgr_telemetry_raise_intf`
     - **7.1.3.** API: `eponMgr_telemetry_raise_alarm`
   - **7.2.** No marker name string literal MUST appear outside `src/telemetry/`.
   - **7.3.** No `t2_event_*` symbol MUST be referenced outside `src/telemetry/`.

8. EPON Manager MUST implement robustness guarantees for the telemetry subsystem.
   - **8.1.** A failure in the T2 backend MUST NOT propagate as a failure to the producer caller; telemetry hiccups MUST NOT cascade.
   - **8.2.** The telemetry module MUST be thread-safe across the event thread and stats-poller thread.

---

## SHOULD (P1 — strongly desired, defer only with justification)

9. EPON Manager SHOULD implement rate-limiting for repeated error-class events.
    - **9.1.** Identical raise-on-error events (e.g. repeated `EPON_ERROR_HAL_CALL_FAILED`) SHOULD be suppressed to no more than one event per second per marker.
      - **9.1.1.** Scope: Applies to all events in section 6 (Runtime errors).
      - **9.1.2.** Rationale: Sustained HAL or RBus failures can fire at very high frequency; rate-limiting prevents T2 backend flooding while still signalling the condition.

10. EPON Manager SHOULD include LLID context in standard alarm values whenever available.
    - **10.1.** The `,LLID=<n>` suffix SHOULD be appended to the alarm value for any standard IEEE 802.3ah alarm (section 3) when the HAL provides a non-`0xFFFF` LLID.
      - **10.1.1.** Scope: Applies to section 3 alarms only; vendor alarms (section 4) always have `LLID=0xFFFF` (not applicable) and MUST NOT include the suffix.
      - **10.1.2.** Rationale: Enables dashboards to correlate degradation to a specific logical link.

---

## COULD (P2 — nice-to-have)

11. EPON Manager COULD record per-marker fired-counts for offline auditing.
    - **11.1.** Output path: `/tmp/eponmanager_telem_stats`.
    - **11.2.** Rationale: Aids post-mortem analysis of event storm incidents without requiring T2 backend access.

12. EPON Manager COULD emit a `Debug` priority event when an unknown HAL alarm enum is received.
    - **12.1.** Priority: Debug
    - **12.2.** Rationale: Aids detection of HAL-version mismatches where new alarm codes are not yet handled by the manager.

---

## WON'T (out of scope for this story)

13. EPON Manager WON'T implement the Harvester periodic Avro report in this story.
    - **13.1.** The `EPONTelemetryDiagnostics` Avro report, Avro encoding, and WebPA/libparodus transport are deferred. See [10_Harvester_Future_Direction.md](10_Harvester_Future_Direction.md).

14. EPON Manager WON'T re-implement per-stat T2 markers.
    - **14.1.** The former 28 per-stat markers are deferred to the future harvester report.

15. This story WON'T alter the TR-181 parameter set.
    - **15.1.** Only telemetry producer code is in scope; data model changes are deferred.

---

## Test evidence (definition of done)

A story is considered **done** when, in addition to a clean
`make check`:

* All 34 event ids defined in
  `eponMgr_telemetry_event_id_t` are observed at least once on the
  T2 stub log when the `tests/hal_mock` driver
  ([epon_hal_trigger.c](../tests/hal_mock/epon_hal_trigger.c)) walks
  every callback path.
* No marker string and no `t2_event_*` symbol is found outside
  `src/telemetry/` (verified via `grep -R` in CI).
* `eponMgr_telemetry_set_enabled(false)` blocks events; flipping
  back to `true` resumes them within one event.
