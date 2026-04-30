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

### M1. ONU status events

* The CPE **MUST** publish an `EPON_ONU_LOS` telemetry event when the
  HAL reports loss of downstream optical signal.
  * Marker value **MUST** be empty (`""`).
  * Priority **MUST** be `Critical`.
  * The event **MUST** be raised within 1 second of the HAL
    `status_callback`.
* The CPE **MUST** publish `EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED` when
  signal is detected but the ONU is not yet registered.
  * Priority **MUST** be `Info`.
* The CPE **MUST** publish `EPON_ONU_REGISTRATION` when the ONU
  completes MPCP discovery + OAM negotiation.
* The CPE **MUST** publish `EPON_ONU_DEREGISTRATION` when the OLT
  deregisters the ONU or MPCP times out.
  * Priority **MUST** be `Warning`.

### M2. Interface link status events

* The CPE **MUST** publish `EPON_INTF_LINK_UP` when an EPON WAN
  interface (e.g. `veip0`) comes up.
  * Marker value **MUST** be `"Interface=<ifname>"`.
* The CPE **MUST** publish `EPON_INTF_LINK_DOWN` when an EPON WAN
  interface goes down.
  * Marker value **MUST** be `"Interface=<ifname>"`.
* The CPE **MUST** publish `EPON_PHY_STATUS_UP` when the first
  interface transitions UP after all were DOWN.
* The CPE **MUST** publish `EPON_PHY_STATUS_DOWN` when the last
  interface transitions DOWN.
  * Priority **MUST** be `Critical`.

### M3. Standard IEEE 802.3ah alarms (RAISED & CLEARED)

For every alarm below, the CPE **MUST** publish one event on RAISED
and one on CLEARED, with marker value `"RAISED"` or `"CLEARED"`.
When the alarm is per-LLID, value **MUST** include `,LLID=<n>`.

* The CPE **MUST** publish `EPON_ALARM_STD_LOFI` when the HAL reports
  Loss-of-Frame/Lock (Critical).
* The CPE **MUST** publish `EPON_ALARM_STD_ERROR_SYMBOL_PERIOD` when
  the Errored Symbol Period threshold is breached (IEEE 802.3ah
  Clause 57.5.2).
  * Priority **MUST** be `Error`.
* The CPE **MUST** publish `EPON_ALARM_STD_ERROR_FRAME` when the
  Errored Frame threshold is breached (Clause 57.5.3).
* The CPE **MUST** publish `EPON_ALARM_STD_ERROR_FRAME_PERIOD` when
  the Errored Frame Period threshold is breached (Clause 57.5.4).
* The CPE **MUST** publish `EPON_ALARM_STD_ERROR_FRAME_SECONDS` when
  the Errored Frame Seconds threshold is breached (Clause 57.5.5).
  * Priority **MUST** be `Warning`.
* The CPE **MUST** publish `EPON_ALARM_STD_OAM_SESSION_LOST` when
  OAM keepalive detects loss of the OAM peer.
  * Priority **MUST** be `Critical`.
* The CPE **MUST** publish `EPON_ALARM_STD_EQUIPMENT_FAILURE` when
  the HAL signals an ONU hardware failure.
  * Priority **MUST** be `Critical`.

### M4. Vendor-specific (DPoE) alarms (RAISED & CLEARED)

For every alarm below, the CPE **MUST** publish one event on RAISED
and one on CLEARED with value `"RAISED"` / `"CLEARED"`.

* The CPE **MUST** publish `EPON_ALARM_VENDOR_LOS` on complete loss
  of optical signal at the receiver (Critical).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_DYING_GASP` on imminent
  power loss before the ONU reboots (Critical).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_POWER_LOW` when RX
  optical power drops below the lower threshold (Warning).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_POWER_HIGH` when RX
  optical power exceeds the upper threshold (Warning).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_TEMPERATURE` when the
  transceiver temperature exceeds the safe threshold (Error).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_FEC_THRESHOLD` when
  uncorrectable FEC errors exceed threshold (Error).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_LASER_BIAS_CURRENT`
  when laser bias current is out of range (Error).
* The CPE **MUST** publish `EPON_ALARM_VENDOR_SUPPLY_VOLTAGE` when
  the transceiver supply voltage is out of nominal range (Error).

### M5. System lifecycle events

* The CPE **MUST** publish `EPON_SYSTEM_INIT_SUCCESS` once after all
  subsystems (RBus, PSM, HAL, stats poller, telemetry, harvester)
  start successfully.
* The CPE **MUST** publish `EPON_SYSTEM_INIT_FAILURE` if any
  subsystem fails to initialize.
  * Priority **MUST** be `Critical`.
* The CPE **MUST** publish `EPON_SYSTEM_SHUTDOWN` on receipt of
  SIGTERM/SIGINT before tearing down.
* The CPE **MUST** publish `EPON_SYSTEM_HAL_WRONG_PON_MODE` when the
  HAL reports the hardware is configured for a non-EPON mode.
  * Priority **MUST** be `Critical`.
* The CPE **MUST** publish `EPON_SYSTEM_FACTORY_RESET` when a
  factory reset is triggered via the TR-181 `FactoryReset` action.
* The CPE **MUST** publish `EPON_SYSTEM_ONU_RESET` when the TR-181
  `Reset` action is invoked.

### M6. Runtime error events

* The CPE **MUST** publish `EPON_ERROR_HAL_CALL_FAILED` when any
  HAL API returns a non-success status.
  * Priority **MUST** be `Error`.
* The CPE **MUST** publish `EPON_ERROR_EVENT_QUEUE_FULL` when the
  internal event queue drops an event.
* The CPE **MUST** publish `EPON_ERROR_STATS_COLLECTION_FAILED`
  when periodic stats retrieval from the HAL fails.
  * Priority **MUST** be `Warning`.
* The CPE **MUST** publish `EPON_ERROR_RBUS_PUBLISH_FAILED` when an
  RBus publish or set fails.
* The CPE **MUST** publish `EPON_ERROR_PSM_ACCESS_FAILED` on PSM
  read/write failure.

### M7. Harvester periodic Avro report

* The CPE **MUST** publish exactly one `EPONTelemetryDiagnostics`
  Avro report every `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.PollingInterval`
  seconds (default 900 s).
  * The report **MUST** validate against `EponReport.avsc`.
  * The frame **MUST** be `[MAGIC=0x8A | UUID(16B) | SCHEMA_MD5(16B) | avro_binary]`.
  * The transport **MUST** be `libparodus_send` to destination
    `event:raw.kestrel.reports.EponReport` with content-type
    `avro/binary`.
* The report header **MUST** populate:
  * `header.timestamp` — current UTC time in millis.
  * `header.uuid` — RFC-4122 v4.
  * `header.source` — `"rdk-eponmanager"`.
  * `cpe_id.mac_address` — 6-byte fixed gateway MAC.
  * `cpe_id.cpe_type` — `"Gateway"`.
* The report `data` payload **MUST** populate the statistics listed
  in M7.1 / M7.2 / M7.3 below, sourced from
  [EPON_Manager_Reference_v2.md §3.1.2–3.1.4](EPON_Manager_Reference_v2.md#312-link-statistics).
  * All optical/voltage/temperature/current values **MUST** be
    converted to the integer Dbm1000 / m°C / mV / µA encoding
    (×1000) prior to encoding.
  * Any field whose source value is unavailable **MUST** be encoded
    as the Avro `null` branch of its union.
* The CPE **MUST NOT** publish a periodic report when
  `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.Enable` is
  `false`.

#### M7.1 Link statistics (source: `epon_hal_get_link_stats()`)

The CPE **MUST** include the following 20 link-statistics fields in
every report (each as its corresponding Avro `union[null, …]` type):

* `data.BytesSent` — total bytes transmitted (bytes, long).
* `data.BytesReceived` — total bytes received (bytes, long).
* `data.PacketsSent` — total packets transmitted (long).
* `data.PacketsReceived` — total packets received (long).
* `data.ErrorsSent` — total transmission errors (long).
* `data.ErrorsReceived` — total reception errors (long).
* `data.DiscardPacketsSent` — packets discarded prior to TX (long).
* `data.DiscardPacketsReceived` — packets discarded on RX (long).
* `data.UnicastPacketsSent` — unicast packets transmitted (long).
* `data.UnicastPacketsReceived` — unicast packets received (long).
* `data.BroadcastPacketsSent` — broadcast packets transmitted (long).
* `data.BroadcastPacketsReceived` — broadcast packets received (long).
* `data.MulticastPacketsSent` — multicast packets transmitted (long).
* `data.MulticastPacketsReceived` — multicast packets received (long).
* `data.MaxBitRate` — negotiated bit rate in Mbps (int; typically
  1000 for 1G-EPON or 10000 for 10G-EPON).
* `data.FECCorrected` — FEC corrected bit error count (long).
* `data.FECUncorrectable` — FEC uncorrectable codeword count (long;
  rising trend = severe signal degradation).
* `data.RangingResyncs` — MPCP ranging resync count (long; rising
  trend = distance / timing instability).
* `data.MACResets` — MAC layer reset count (long; rising trend =
  link-level recovery events).
* `data.BER` — calculated Bit Error Rate, scientific notation
  string, e.g. `"1.5e-09"`.

#### M7.2 Transceiver / optical statistics (source: `epon_hal_get_transceiver_stats()`)

The CPE **MUST** include the following 9 transceiver fields. All
values **MUST** use ×1000 fixed-point integer encoding (Dbm1000 /
m°C / mV / µA):

* `data.TransmitOpticalLevel` — TX optical power, Dbm1000 (typical
  −6000 to +3000).
* `data.OpticalSignalLevel` — RX optical power, Dbm1000 (typical
  −30000 to 0).
* `data.LowerOpticalThreshold` — lower RX optical alarm threshold,
  Dbm1000.
* `data.UpperOpticalThreshold` — upper RX optical alarm threshold,
  Dbm1000.
* `data.LowerTransmitPowerThreshold` — lower TX optical alarm
  threshold, Dbm1000.
* `data.UpperTransmitPowerThreshold` — upper TX optical alarm
  threshold, Dbm1000.
* `data.TransceiverTemperature` — module temperature, m°C
  (e.g. `45200` = 45.2 °C).
* `data.SupplyVoltage` — transceiver supply voltage, mV
  (typical ~`3300`).
* `data.LaserBiasCurrent` — laser bias current, µA (rising trend
  indicates laser aging).

#### M7.3 ONU status (source: `epon_hal_get_onu_status()` / `eponMgr_onu_state_t`)

The CPE **MUST** include the following 3 ONU-status fields:

* `data.OperationalMode` — string, e.g. `"1G-EPON"` or `"10G-EPON"`.
* `data.EncryptionMode` — int: `0`=Disabled, `1`=AES-128,
  `2`=Triple Churning, `3`=AES-256.
* `data.ONUStatus` — int: `0`=LOS, `1`=Downstream Signal Detected,
  `2`=Registered, `3`=Deregistering.

> Total Avro `data` fields: **32** (20 link + 9 transceiver + 3 ONU).
> All 32 are wrapped in `union[null, T]` so any unavailable value
> serializes as `null`.

### M8. Single-producer surface

* All telemetry calls **MUST** go through one of the three public
  APIs declared in [`include/eponMgr_telemetry.h`](../include/eponMgr_telemetry.h):
  `eponMgr_telemetry_raise_simple`, `_raise_intf`, `_raise_alarm`.
  * No marker name string literal **MUST** appear outside
    `src/telemetry/`.
  * No `t2_event_*` symbol **MUST** be referenced outside
    `src/telemetry/`.
  * No `avro_*` or `libparodus_*` symbol **MUST** be referenced
    outside `src/telemetry/`.

### M9. Robustness

* A failure in the T2 backend **MUST NOT** propagate as a failure
  to the producer caller (telemetry hiccups MUST NOT cascade).
* A failure in `libparodus_send` **MUST** drop the affected report
  only; the next interval **MUST** still attempt publish.
* The telemetry module **MUST** be thread-safe across the event
  thread, stats-poller thread, and harvester thread.

---

## SHOULD (P1 — strongly desired, defer only with justification)

* The CPE **SHOULD** rate-limit identical raise-on-error events
  (e.g. repeated `EPON_ERROR_HAL_CALL_FAILED`) to no more than
  one event per second per marker.
* The CPE **SHOULD** include `,LLID=<n>` in the alarm value
  whenever the HAL provides a non-`0xFFFF` LLID, so dashboards
  can correlate per-LLID degradation.
* The harvester **SHOULD** compute the schema MD5 once at startup
  by hashing `/usr/ccsp/harvester/EponReport.avsc` and reuse it
  for every report.
* The harvester thread **SHOULD** use `pthread_cond_timedwait`
  (not `sleep`) so a shutdown signal interrupts the wait promptly.
* The CPE **SHOULD** continue to deliver telemetry events even
  when the harvester is disabled, and vice versa.
* `EponReport.avsc` **SHOULD** be installed at
  `/usr/ccsp/harvester/EponReport.avsc` by the build system.

---

## COULD (P2 — nice-to-have)

* The CPE **COULD** expose a debug TR-181 parameter to force an
  immediate harvester publish (useful for field diagnosis).
* The CPE **COULD** record per-marker fired-counts in
  `/tmp/eponmanager_telem_stats` for offline auditing.
* The harvester **COULD** support multiple destinations
  (additional `wrp_msg_t` `dest`) configured via PSM.
* The CPE **COULD** emit a `Debug` priority event when an unknown
  HAL alarm enum is received, to aid HAL-version mismatch
  detection.

---

## WON'T (out of scope for this story)

* The CPE **WON'T** re-implement per-stat T2 markers that were
  superseded by the harvester report (former Reference §6 28
  markers — now folded into `EPONTelemetryDiagnostics`).
* The CPE **WON'T** persist failed harvester reports for retry
  beyond the next polling interval.
* The CPE **WON'T** implement custom WebPA framing — it reuses
  the same `[MAGIC | UUID | MD5 | avro]` scheme as
  `rdk-xdslmanager`.
* The CPE **WON'T** ship its own Avro encoder — it links against
  the SDK-provided `libavro-c`.
* This story **WON'T** alter the TR-181 parameter set; only
  telemetry/harvester producer code is changed.

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
