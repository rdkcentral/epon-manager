# EPON Manager — Telemetry & Harvester Acceptance Criteria

**Version:** 1.0  |  **Date:** April 30, 2026
**Jira Story:** EPON Manager — Telemetry & Harvester Implementation
**Format:** MoSCoW (Must / Should / Could / Won't)
**Source-of-truth spec:** [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) §2 / §3
**Companion:** [07_Telemetry_Implementation_Plan.md](07_Telemetry_Implementation_Plan.md), [08_Telemetry_Design.md](08_Telemetry_Design.md)

---

## Conventions

* "**CPE**" = the gateway running `rdk-eponmanager`.
* "Telemetry event" = a discrete T2 marker dispatched via
  `t2_event_s(marker, value)`.
* "Harvester report" = one binary Avro `EPONTelemetryDiagnostics`
  record framed as `[MAGIC | UUID | SCHEMA_MD5 | avro_binary]` and
  shipped via `libparodus_send`.
* "Within X seconds" is wall-clock from the originating HAL callback
  to the T2/WebPA send call.

---

## MUST (P0 — release blockers)

1. EPON Manager MUST send T2 events to monitor ONU status changes.
    1.1. EPON Manager MUST send a T2 event when the HAL reports loss of downstream optical signal.
        1.1.1. Event Name: `EPON_ONU_LOS`
        1.1.2. Event Arguments(VALUE): `""` (empty string)
        1.1.3. Priority: Critical
        1.1.4. Timing: MUST be raised within 1 second of the HAL `status_callback`.
        1.1.5. Rate Limit: No rate limit; state-change driven (HAL callback).
    1.2. EPON Manager MUST send a T2 event when downstream signal is detected but the ONU is not yet registered.
        1.2.1. Event Name: `EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED`
        1.2.2. Event Arguments(VALUE): `""` (empty string)
        1.2.3. Priority: Info
        1.2.4. Rate Limit: No rate limit; state-change driven (HAL callback).
    1.3. EPON Manager MUST send a T2 event when the ONU completes MPCP discovery and OAM negotiation.
        1.3.1. Event Name: `EPON_ONU_REGISTRATION`
        1.3.2. Event Arguments(VALUE): `""` (empty string)
        1.3.3. Priority: Info
        1.3.4. Rate Limit: No rate limit; state-change driven (HAL callback).
    1.4. EPON Manager MUST send a T2 event when the OLT deregisters the ONU or MPCP times out.
        1.4.1. Event Name: `EPON_ONU_DEREGISTRATION`
        1.4.2. Event Arguments(VALUE): `""` (empty string)
        1.4.3. Priority: Warning
        1.4.4. Rate Limit: No rate limit; state-change driven (HAL callback).

2. EPON Manager MUST send T2 events to monitor EPON WAN interface link status.
    2.1. EPON Manager MUST send a T2 event when an EPON WAN interface comes up.
        2.1.1. Event Name: `EPON_INTF_LINK_UP`
        2.1.2. Event Arguments(INTERFACE): `Interface=<ifname>` (e.g. `Interface=veip0`)
        2.1.3. Priority: Info
        2.1.4. Rate Limit: No rate limit; state-change driven (interface link event).
    2.2. EPON Manager MUST send a T2 event when an EPON WAN interface goes down.
        2.2.1. Event Name: `EPON_INTF_LINK_DOWN`
        2.2.2. Event Arguments(INTERFACE): `Interface=<ifname>` (e.g. `Interface=veip0`)
        2.2.3. Priority: Warning
        2.2.4. Rate Limit: No rate limit; state-change driven (interface link event).
    2.3. EPON Manager MUST send a T2 event when the first interface transitions UP after all were DOWN.
        2.3.1. Event Name: `EPON_PHY_STATUS_UP`
        2.3.2. Event Arguments(VALUE): `""` (empty string)
        2.3.3. Priority: Info
        2.3.4. Rate Limit: No rate limit; state-change driven (aggregate PHY transition).
    2.4. EPON Manager MUST send a T2 event when the last interface transitions DOWN.
        2.4.1. Event Name: `EPON_PHY_STATUS_DOWN`
        2.4.2. Event Arguments(VALUE): `""` (empty string)
        2.4.3. Priority: Critical
        2.4.4. Rate Limit: No rate limit; state-change driven (aggregate PHY transition).

3. EPON Manager MUST send T2 events for Standard IEEE 802.3ah alarms on both RAISED and CLEARED transitions. When the alarm is per-LLID, the value MUST include `,LLID=<n>`.
    3.1. EPON Manager MUST send a T2 event for Loss-of-Frame/Lock.
        3.1.1. Event Name: `EPON_ALARM_STD_LOFI`
        3.1.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.1.3. Priority: Critical
        3.1.4. Rate Limit: No rate limit; state-change driven (alarm raised/cleared transition).
    3.2. EPON Manager MUST send a T2 event when the Errored Symbol Period threshold is breached (IEEE 802.3ah Clause 57.5.2).
        3.2.1. Event Name: `EPON_ALARM_STD_ERROR_SYMBOL_PERIOD`
        3.2.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.2.3. Priority: Error
        3.2.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    3.3. EPON Manager MUST send a T2 event when the Errored Frame threshold is breached (Clause 57.5.3).
        3.3.1. Event Name: `EPON_ALARM_STD_ERROR_FRAME`
        3.3.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.3.3. Priority: Warning
        3.3.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    3.4. EPON Manager MUST send a T2 event when the Errored Frame Period threshold is breached (Clause 57.5.4).
        3.4.1. Event Name: `EPON_ALARM_STD_ERROR_FRAME_PERIOD`
        3.4.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.4.3. Priority: Warning
        3.4.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    3.5. EPON Manager MUST send a T2 event when the Errored Frame Seconds threshold is breached (Clause 57.5.5).
        3.5.1. Event Name: `EPON_ALARM_STD_ERROR_FRAME_SECONDS`
        3.5.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.5.3. Priority: Warning
        3.5.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    3.6. EPON Manager MUST send a T2 event when OAM keepalive detects loss of the OAM peer.
        3.6.1. Event Name: `EPON_ALARM_STD_OAM_SESSION_LOST`
        3.6.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.6.3. Priority: Critical
        3.6.4. Rate Limit: No rate limit; state-change driven (OAM session event).
    3.7. EPON Manager MUST send a T2 event when the HAL signals an ONU hardware failure.
        3.7.1. Event Name: `EPON_ALARM_STD_EQUIPMENT_FAILURE`
        3.7.2. Event Arguments(STATE[,LLID]): `RAISED`, `CLEARED`; append `,LLID=<n>` for per-LLID alarms.
        3.7.3. Priority: Critical
        3.7.4. Rate Limit: No rate limit; state-change driven (HAL hardware fault).

4. EPON Manager MUST send T2 events for Vendor-Specific (DPoE) alarms on both RAISED and CLEARED transitions. These are physical/hardware-layer events scoped to the entire ONU transceiver; they carry no LLID because the HAL always returns `LLID=0xFFFF` (not applicable) for hardware-level conditions.
    4.1. EPON Manager MUST send a T2 event on complete loss of optical signal at the receiver.
        4.1.1. Event Name: `EPON_ALARM_VENDOR_LOS`
        4.1.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.1.3. Priority: Critical
        4.1.4. Rate Limit: No rate limit; state-change driven (optical loss event).
    4.2. EPON Manager MUST send a T2 event on imminent power loss before the ONU reboots.
        4.2.1. Event Name: `EPON_ALARM_VENDOR_DYING_GASP`
        4.2.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.2.3. Priority: Critical
        4.2.4. Rate Limit: No rate limit; fired once per power-loss event.
    4.3. EPON Manager MUST send a T2 event when RX optical power drops below the lower threshold.
        4.3.1. Event Name: `EPON_ALARM_VENDOR_POWER_LOW`
        4.3.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.3.3. Priority: Warning
        4.3.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    4.4. EPON Manager MUST send a T2 event when RX optical power exceeds the upper threshold.
        4.4.1. Event Name: `EPON_ALARM_VENDOR_POWER_HIGH`
        4.4.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.4.3. Priority: Warning
        4.4.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    4.5. EPON Manager MUST send a T2 event when the transceiver temperature exceeds the safe threshold.
        4.5.1. Event Name: `EPON_ALARM_VENDOR_TEMPERATURE`
        4.5.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.5.3. Priority: Error
        4.5.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    4.6. EPON Manager MUST send a T2 event when uncorrectable FEC errors exceed the threshold.
        4.6.1. Event Name: `EPON_ALARM_VENDOR_FEC_THRESHOLD`
        4.6.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.6.3. Priority: Error
        4.6.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    4.7. EPON Manager MUST send a T2 event when laser bias current is out of range.
        4.7.1. Event Name: `EPON_ALARM_VENDOR_LASER_BIAS_CURRENT`
        4.7.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.7.3. Priority: Error
        4.7.4. Rate Limit: No rate limit; state-change driven (threshold crossing).
    4.8. EPON Manager MUST send a T2 event when the transceiver supply voltage is out of nominal range.
        4.8.1. Event Name: `EPON_ALARM_VENDOR_SUPPLY_VOLTAGE`
        4.8.2. Event Arguments(STATE): `RAISED`, `CLEARED`
        4.8.3. Priority: Error
        4.8.4. Rate Limit: No rate limit; state-change driven (threshold crossing).

5. EPON Manager MUST send T2 events for system lifecycle transitions.
    5.1. EPON Manager MUST send a T2 event once after all subsystems (RBus, PSM, HAL, stats poller, telemetry, harvester) start successfully.
        5.1.1. Event Name: `EPON_SYSTEM_INIT_SUCCESS`
        5.1.2. Event Arguments(VALUE): `""` (empty string)
        5.1.3. Priority: Info
        5.1.4. Rate Limit: Fired once per process start; structurally non-repeatable.
    5.2. EPON Manager MUST send a T2 event if any subsystem fails to initialize.
        5.2.1. Event Name: `EPON_SYSTEM_INIT_FAILURE`
        5.2.2. Event Arguments(VALUE): `""` (empty string)
        5.2.3. Priority: Critical
        5.2.4. Rate Limit: Fired once per process start; structurally non-repeatable.
    5.3. EPON Manager MUST send a T2 event on receipt of SIGTERM/SIGINT before tearing down.
        5.3.1. Event Name: `EPON_SYSTEM_SHUTDOWN`
        5.3.2. Event Arguments(VALUE): `""` (empty string)
        5.3.3. Priority: Info
        5.3.4. Rate Limit: Fired once per shutdown signal; structurally non-repeatable.
    5.4. EPON Manager MUST send a T2 event when the HAL reports hardware configured for a non-EPON mode.
        5.4.1. Event Name: `EPON_SYSTEM_HAL_WRONG_PON_MODE`
        5.4.2. Event Arguments(VALUE): `""` (empty string)
        5.4.3. Priority: Critical
        5.4.4. Rate Limit: No rate limit; state-change driven (HAL callback).
    5.5. EPON Manager MUST send a T2 event when a factory reset is triggered via the TR-181 `FactoryReset` action.
        5.5.1. Event Name: `EPON_SYSTEM_FACTORY_RESET`
        5.5.2. Event Arguments(VALUE): `""` (empty string)
        5.5.3. Priority: Warning
        5.5.4. Rate Limit: No rate limit; operator-initiated action.
    5.6. EPON Manager MUST send a T2 event when the TR-181 `Reset` action is invoked.
        5.6.1. Event Name: `EPON_SYSTEM_ONU_RESET`
        5.6.2. Event Arguments(VALUE): `""` (empty string)
        5.6.3. Priority: Info
        5.6.4. Rate Limit: No rate limit; operator-initiated action.

6. EPON Manager MUST send T2 events for runtime errors.
    6.1. EPON Manager MUST send a T2 event when any HAL API returns a non-success status.
        6.1.1. Event Name: `EPON_ERROR_HAL_CALL_FAILED`
        6.1.2. Event Arguments(VALUE): `""` (empty string)
        6.1.3. Priority: Error
        6.1.4. Rate Limit: Max 1 event per second per marker; prevents flooding on sustained HAL failures.
    6.2. EPON Manager MUST send a T2 event when the internal event queue drops an event.
        6.2.1. Event Name: `EPON_ERROR_EVENT_QUEUE_FULL`
        6.2.2. Event Arguments(VALUE): `""` (empty string)
        6.2.3. Priority: Error
        6.2.4. Rate Limit: Max 1 event per second per marker.
    6.3. EPON Manager MUST send a T2 event when periodic stats retrieval from the HAL fails.
        6.3.1. Event Name: `EPON_ERROR_STATS_COLLECTION_FAILED`
        6.3.2. Event Arguments(VALUE): `""` (empty string)
        6.3.3. Priority: Warning
        6.3.4. Rate Limit: Max 1 event per `StatsPoller.PollingInterval`; naturally bounded by polling cadence.
    6.4. EPON Manager MUST send a T2 event when an RBus publish or set fails.
        6.4.1. Event Name: `EPON_ERROR_RBUS_PUBLISH_FAILED`
        6.4.2. Event Arguments(VALUE): `""` (empty string)
        6.4.3. Priority: Error
        6.4.4. Rate Limit: Max 1 event per second per marker.
    6.5. EPON Manager MUST send a T2 event on PSM read/write failure.
        6.5.1. Event Name: `EPON_ERROR_PSM_ACCESS_FAILED`
        6.5.2. Event Arguments(VALUE): `""` (empty string)
        6.5.3. Priority: Error
        6.5.4. Rate Limit: Max 1 event per second per marker.

7. EPON Manager MUST publish periodic binary Avro harvester reports (`EPONTelemetryDiagnostics`).
    7.1. EPON Manager MUST publish exactly one report per `StatsPoller.PollingInterval` seconds (default 900 s).
        7.1.1. Schema: MUST validate against `EponReport.avsc`.
        7.1.2. Frame Format: `[MAGIC=0x8A | UUID(16B) | SCHEMA_MD5(16B) | avro_binary]`.
        7.1.3. Transport: `libparodus_send` to `event:raw.kestrel.reports.EponReport`, content-type `avro/binary`.
        7.1.4. Rate Limit: Exactly one report per `PollingInterval`; suppressed entirely when `StatsPoller.Enable=false`.
    7.2. EPON Manager MUST populate the report header.
        7.2.1. `header.timestamp` — current UTC time in milliseconds.
        7.2.2. `header.uuid` — RFC-4122 v4 UUID.
        7.2.3. `header.source` — `"rdk-eponmanager"`.
        7.2.4. `cpe_id.mac_address` — 6-byte fixed gateway MAC.
        7.2.5. `cpe_id.cpe_type` — `"Gateway"`.
    7.3. EPON Manager MUST populate 20 link-statistics fields sourced from `epon_hal_get_link_stats()`. All fields MUST be wrapped in `union[null, T]`; unavailable values MUST be encoded as the Avro `null` branch.
        7.3.1. `data.BytesSent` — total bytes transmitted (long).
        7.3.2. `data.BytesReceived` — total bytes received (long).
        7.3.3. `data.PacketsSent` — total packets transmitted (long).
        7.3.4. `data.PacketsReceived` — total packets received (long).
        7.3.5. `data.ErrorsSent` — total transmission errors (long).
        7.3.6. `data.ErrorsReceived` — total reception errors (long).
        7.3.7. `data.DiscardPacketsSent` — packets discarded prior to TX (long).
        7.3.8. `data.DiscardPacketsReceived` — packets discarded on RX (long).
        7.3.9. `data.UnicastPacketsSent` — unicast packets transmitted (long).
        7.3.10. `data.UnicastPacketsReceived` — unicast packets received (long).
        7.3.11. `data.BroadcastPacketsSent` — broadcast packets transmitted (long).
        7.3.12. `data.BroadcastPacketsReceived` — broadcast packets received (long).
        7.3.13. `data.MulticastPacketsSent` — multicast packets transmitted (long).
        7.3.14. `data.MulticastPacketsReceived` — multicast packets received (long).
        7.3.15. `data.MaxBitRate` — negotiated bit rate in Mbps (int; `1000` for 1G-EPON, `10000` for 10G-EPON).
        7.3.16. `data.FECCorrected` — FEC corrected bit error count (long).
        7.3.17. `data.FECUncorrectable` — FEC uncorrectable codeword count (long).
        7.3.18. `data.RangingResyncs` — MPCP ranging resync count (long).
        7.3.19. `data.MACResets` — MAC layer reset count (long).
        7.3.20. `data.BER` — calculated Bit Error Rate, scientific notation string (e.g. `"1.5e-09"`).
    7.4. EPON Manager MUST populate 9 transceiver/optical fields sourced from `epon_hal_get_transceiver_stats()`. All values MUST use ×1000 fixed-point integer encoding (Dbm1000 / m°C / mV / µA). All fields MUST be wrapped in `union[null, T]`.
        7.4.1. `data.TransmitOpticalLevel` — TX optical power, Dbm1000 (typical −6000 to +3000).
        7.4.2. `data.OpticalSignalLevel` — RX optical power, Dbm1000 (typical −30000 to 0).
        7.4.3. `data.LowerOpticalThreshold` — lower RX optical alarm threshold, Dbm1000.
        7.4.4. `data.UpperOpticalThreshold` — upper RX optical alarm threshold, Dbm1000.
        7.4.5. `data.LowerTransmitPowerThreshold` — lower TX optical alarm threshold, Dbm1000.
        7.4.6. `data.UpperTransmitPowerThreshold` — upper TX optical alarm threshold, Dbm1000.
        7.4.7. `data.TransceiverTemperature` — module temperature, m°C (e.g. `45200` = 45.2 °C).
        7.4.8. `data.SupplyVoltage` — transceiver supply voltage, mV (typical ~`3300`).
        7.4.9. `data.LaserBiasCurrent` — laser bias current, µA.
    7.5. EPON Manager MUST populate 3 ONU-status fields sourced from `epon_hal_get_onu_status()` / `eponMgr_onu_state_t`. All fields MUST be wrapped in `union[null, T]`.
        7.5.1. `data.OperationalMode` — string: `"1G-EPON"` or `"10G-EPON"`.
        7.5.2. `data.EncryptionMode` — int: `0`=Disabled, `1`=AES-128, `2`=Triple Churning, `3`=AES-256.
        7.5.3. `data.ONUStatus` — int: `0`=LOS, `1`=Downstream Signal Detected, `2`=Registered, `3`=Deregistering.

8. EPON Manager MUST enforce a single-producer surface for all telemetry output.
    8.1. All telemetry calls MUST go through one of the three public APIs declared in [`include/eponMgr_telemetry.h`](../include/eponMgr_telemetry.h).
        8.1.1. API: `eponMgr_telemetry_raise_simple`
        8.1.2. API: `eponMgr_telemetry_raise_intf`
        8.1.3. API: `eponMgr_telemetry_raise_alarm`
    8.2. No marker name string literal MUST appear outside `src/telemetry/`.
    8.3. No `t2_event_*` symbol MUST be referenced outside `src/telemetry/`.
    8.4. No `avro_*` or `libparodus_*` symbol MUST be referenced outside `src/telemetry/`.

9. EPON Manager MUST implement robustness guarantees for the telemetry subsystem.
    9.1. A failure in the T2 backend MUST NOT propagate as a failure to the producer caller; telemetry hiccups MUST NOT cascade.
    9.2. A failure in `libparodus_send` MUST drop the affected report only; the next interval MUST still attempt publish.
    9.3. The telemetry module MUST be thread-safe across the event thread, stats-poller thread, and harvester thread.

---

## SHOULD (P1 — strongly desired, defer only with justification)

10. EPON Manager SHOULD implement rate-limiting for repeated error-class events.
    10.1. Identical raise-on-error events (e.g. repeated `EPON_ERROR_HAL_CALL_FAILED`) SHOULD be suppressed to no more than one event per second per marker.
        10.1.1. Scope: Applies to all events in section 6 (Runtime errors).
        10.1.2. Rationale: Sustained HAL or RBus failures can fire at very high frequency; rate-limiting prevents T2 backend flooding while still signalling the condition.

11. EPON Manager SHOULD include LLID context in standard alarm values whenever available.
    11.1. The `,LLID=<n>` suffix SHOULD be appended to the alarm value for any standard IEEE 802.3ah alarm (section 3) when the HAL provides a non-`0xFFFF` LLID.
        11.1.1. Scope: Applies to section 3 alarms only; vendor alarms (section 4) always have `LLID=0xFFFF` (not applicable) and MUST NOT include the suffix.
        11.1.2. Rationale: Enables dashboards to correlate degradation to a specific logical link.

12. EPON Manager SHOULD optimise harvester schema MD5 computation.
    12.1. The harvester SHOULD compute the schema MD5 once at startup by hashing `/usr/ccsp/harvester/EponReport.avsc` and reuse the result for every report.
        12.1.1. Rationale: Avoids redundant I/O and CPU on every 900 s interval.

13. EPON Manager SHOULD use `pthread_cond_timedwait` in the harvester thread.
    13.1. The harvester thread SHOULD use `pthread_cond_timedwait` (not `sleep`) for its inter-report wait.
        13.1.1. Rationale: Allows a shutdown signal to interrupt the wait promptly without waiting up to `PollingInterval` seconds.

14. EPON Manager SHOULD keep telemetry events and harvester reports independently operable.
    14.1. T2 event delivery SHOULD continue even when the harvester is disabled, and harvester publishes SHOULD continue even when the event subsystem encounters errors.
        14.1.1. Rationale: Prevents a single subsystem fault from silencing all diagnostic output.

15. The build system SHOULD install `EponReport.avsc` to `/usr/ccsp/harvester/EponReport.avsc`.
    15.1. Rationale: Required path for the harvester MD5-at-startup and for field validation with `avro-tools`.

---

## COULD (P2 — nice-to-have)

16. EPON Manager COULD expose a debug TR-181 parameter to force an immediate harvester publish.
    16.1. Rationale: Useful for field diagnosis without waiting for the next `PollingInterval`.

17. EPON Manager COULD record per-marker fired-counts for offline auditing.
    17.1. Output path: `/tmp/eponmanager_telem_stats`.
    17.2. Rationale: Aids post-mortem analysis of event storm incidents without requiring T2 backend access.

18. The harvester COULD support multiple publish destinations.
    18.1. Additional `wrp_msg_t` `dest` values COULD be configured via PSM.
    18.2. Rationale: Enables forwarding to secondary analytics endpoints without a code change.

19. EPON Manager COULD emit a `Debug` priority event when an unknown HAL alarm enum is received.
    19.1. Priority: Debug
    19.2. Rationale: Aids detection of HAL-version mismatches where new alarm codes are not yet handled by the manager.

---

## WON'T (out of scope for this story)

20. EPON Manager WON'T re-implement per-stat T2 markers superseded by the harvester report.
    20.1. The former Reference §6 28 per-stat markers are folded into `EPONTelemetryDiagnostics` and are not re-emitted individually.

21. EPON Manager WON'T persist failed harvester reports for retry.
    21.1. A failed `libparodus_send` drops the affected report; the next `PollingInterval` attempt proceeds normally.

22. EPON Manager WON'T implement custom WebPA framing.
    22.1. The existing `[MAGIC | UUID | MD5 | avro]` framing scheme shared with `rdk-xdslmanager` is reused as-is.

23. EPON Manager WON'T ship its own Avro encoder.
    23.1. Encoding is delegated to the SDK-provided `libavro-c`.

24. This story WON'T alter the TR-181 parameter set.
    24.1. Only telemetry and harvester producer code is in scope; data model changes are deferred.

---

## Test evidence (definition of done)

A story is considered **done** when, in addition to a clean
`make check`:

* All 34 event ids defined in
  `eponMgr_telemetry_event_id_t` are observed at least once on the
  T2 stub log when the `tests/hal_mock` driver
  ([epon_hal_trigger.c](../tests/hal_mock/epon_hal_trigger.c)) walks
  every callback path.
* Exactly one Avro record per `PollingInterval` is observed on
  the publisher log, and the captured bytes validate against
  `EponReport.avsc` using `avro-tools` (or equivalent).
* No marker string, no `avro_*` symbol, and no `libparodus_*`
  symbol is found outside `src/telemetry/` (verified via
  `grep -R` in CI).
* `eponMgr_telemetry_set_enabled(false)` blocks events; flipping
  back to `true` resumes them within one event.
* `StatsPoller.Enable=false` blocks harvester publishes.
