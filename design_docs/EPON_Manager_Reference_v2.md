# EPON Manager — TR-181 & Telemetry Reference

**Version:** 1.0 &nbsp;|&nbsp; **Date:** April 17, 2026 &nbsp;|&nbsp; **Component:** RDK EPON Manager (`rdk-eponmanager`)

---

## Table of Contents

1. [TR-181 Data Model Parameters](#1-tr-181-data-model-parameters)
   1. [Base Interface Parameters](#11-base-interface-parameters)
   2. [Optical Parameters](#12-optical-parameters)
   3. [Standard Interface Statistics](#13-standard-interface-statistics)
   4. [X\_RDK Extended Statistics](#14-x_rdk-extended-statistics)
   5. [X\_RDK\_Transceiver Parameters](#15-x_rdk_transceiver-parameters)
   6. [X\_RDK\_EPON Parameters](#16-x_rdk_epon-parameters)
   7. [Manufacturer Information](#17-manufacturer-information)
   8. [OLT Information](#18-olt-information)
   9. [LLID Dynamic Table](#19-llid-dynamic-table)
   10. [DPoE / CPE Dynamic Table](#110-dpoe--cpe-dynamic-table)
   11. [VEIP Interface Dynamic Table](#111-veip-interface-dynamic-table)
   12. [Stats Poller Configuration](#112-stats-poller-configuration)
   13. [ONU Control Actions](#113-onu-control-actions)
2. [Telemetry Events](#2-telemetry-events)
   1. [ONU Status Events](#21-onu-status-events)
   2. [Interface Link Status Events](#22-interface-link-status-events)
   3. [Standard IEEE 802.3ah Alarms](#23-standard-ieee-80233ah-alarms)
   4. [Vendor-Specific Alarms (DPoE)](#24-vendor-specific-alarms-dpoe)
   5. [System Lifecycle Events](#25-system-lifecycle-events)
   6. [Error Events](#26-error-events)
   7. [Priority Definitions](#27-priority-definitions)
   8. [Summary Statistics](#28-summary-statistics)

---

## 1. TR-181 Data Model Parameters

All parameters reside under `Device.Optical.Interface.1` and are registered with RBUS at startup. Dynamic table rows (`{i}`) are registered on demand as the HAL reports new instances.

> **Legend:** `RO` = Read-Only &nbsp;|&nbsp; `RW` = Read/Write &nbsp;|&nbsp; `Table` = Dynamic table placeholder

---

### 1.1 Base Interface Parameters

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 1 | `Device.Optical.Interface.1.Enable` | Administrative enable/disable of the EPON optical interface. | boolean | RW |
| 2 | `Device.Optical.Interface.1.Status` | Operational status derived from ONU state. Maps from `epon_onu_status_t`: _Up_, _Down_, _NotPresent_, _Dormant_, _Unknown_. | string | RO |
| 3 | `Device.Optical.Interface.1.Alias` | User-assignable alias for the optical interface instance. Default: _OpticalInterface1_. | string | RW |
| 4 | `Device.Optical.Interface.1.Name` | Underlying OS interface name (e.g., _veip0_). | string | RO |
| 5 | `Device.Optical.Interface.1.LastChange` | Seconds since the interface last changed status. | uint32 | RO |
| 6 | `Device.Optical.Interface.1.LowerLayers` | Comma-separated list of lower-layer interface paths (empty for EPON). | string | RO |
| 7 | `Device.Optical.Interface.1.Upstream` | Whether this interface is an upstream interface. Always _false_ for ONU. | boolean | RO |
| 8 | `Device.Optical.Interface.1.MaxBitRate` | Maximum negotiated bit rate in Mbps. −1 = auto-negotiate. Typically 1000 (1G-EPON) or 10000 (10G-EPON). | int32 | RO |

---

### 1.2 Optical Parameters

Per BBF TR-181 v2.18. Threshold parameters moved to `X_RDK_Transceiver`.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 9 | `Device.Optical.Interface.1.OpticalSignalLevel` | Receive (RX) optical power in dBm × 1000 (Dbm1000 units). Typical range: −30 000 to 0. | int32 | RO |
| 10 | `Device.Optical.Interface.1.TransmitOpticalLevel` | Transmit (TX) optical power in dBm × 1000 (Dbm1000 units). Typical range: −6 000 to +3 000. | int32 | RO |
| 11 | `Device.Optical.Interface.1.SFPReferenceList` | Reference to SFP/SFP+ pluggable transceiver instance. Currently empty. | string | RO |

---

### 1.3 Standard Interface Statistics

Per BBF TR-181 `Device.Optical.Interface.{i}.Stats`.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 12 | `Device.Optical.Interface.1.Stats.Reset` | Set to _true_ to reset all statistics counters to zero. GET always returns _false_. | boolean | RW |
| 13 | `Device.Optical.Interface.1.Stats.BytesSent` | Total bytes transmitted on the EPON optical interface. | uint64 | RO |
| 14 | `Device.Optical.Interface.1.Stats.BytesReceived` | Total bytes received on the EPON optical interface. | uint64 | RO |
| 15 | `Device.Optical.Interface.1.Stats.PacketsSent` | Total packets transmitted. | uint64 | RO |
| 16 | `Device.Optical.Interface.1.Stats.PacketsReceived` | Total packets received. | uint64 | RO |
| 17 | `Device.Optical.Interface.1.Stats.ErrorsSent` | Total transmission errors. | uint32 | RO |
| 18 | `Device.Optical.Interface.1.Stats.ErrorsReceived` | Total reception errors. | uint32 | RO |
| 19 | `Device.Optical.Interface.1.Stats.DiscardPacketsSent` | Packets discarded prior to transmission. | uint32 | RO |
| 20 | `Device.Optical.Interface.1.Stats.DiscardPacketsReceived` | Packets discarded on reception. | uint32 | RO |

---

### 1.4 X\_RDK Extended Statistics

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 21 | `Device.Optical.Interface.1.Stats.X_RDK_FECCorrected` | FEC corrected bit error count. | uint64 | RO |
| 22 | `Device.Optical.Interface.1.Stats.X_RDK_FECUncorrectable` | FEC uncorrectable codeword count. | uint64 | RO |
| 23 | `Device.Optical.Interface.1.Stats.X_RDK_BER` | Calculated Bit Error Rate (scientific notation string). | string | RO |
| 24 | `Device.Optical.Interface.1.Stats.X_RDK_BroadcastPacketsSent` | Broadcast packets transmitted. | uint64 | RO |
| 25 | `Device.Optical.Interface.1.Stats.X_RDK_BroadcastPacketsReceived` | Broadcast packets received. | uint64 | RO |
| 26 | `Device.Optical.Interface.1.Stats.X_RDK_MulticastPacketsSent` | Multicast packets transmitted. | uint64 | RO |
| 27 | `Device.Optical.Interface.1.Stats.X_RDK_MulticastPacketsReceived` | Multicast packets received. | uint64 | RO |
| 28 | `Device.Optical.Interface.1.Stats.X_RDK_UnicastPacketsSent` | Unicast packets transmitted. | uint64 | RO |
| 29 | `Device.Optical.Interface.1.Stats.X_RDK_UnicastPacketsReceived` | Unicast packets received. | uint64 | RO |
| 30 | `Device.Optical.Interface.1.Stats.X_RDK_RangingResyncs` | MPCP ranging resynchronisation count. | uint32 | RO |
| 31 | `Device.Optical.Interface.1.Stats.X_RDK_MACResets` | MAC layer reset count. | uint32 | RO |

---

### 1.5 X\_RDK\_Transceiver Parameters

Optical thresholds relocated here per BBF TR-181 v2.18 (removed from standard path). All values are represented as signed integers with fixed-point scaling (×1000) since BBF TR-181 does not support floating-point types.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 32 | `Device.Optical.Interface.1.X_RDK_Transceiver.Temperature` | Optical transceiver module temperature in millidegrees Celsius (m°C). Value = °C × 1000. Example: 45200 = 45.2 °C. | int32 | RO |
| 33 | `Device.Optical.Interface.1.X_RDK_Transceiver.SupplyVoltage` | Transceiver supply voltage in millivolts (mV). Value = V × 1000. Example: 3300 = 3.3 V. | int32 | RO |
| 34 | `Device.Optical.Interface.1.X_RDK_Transceiver.BiasCurrent` | Laser bias current in microamps (µA). Value = mA × 1000. Rising trend indicates laser aging. Example: 25500 = 25.5 mA. | int32 | RO |
| 35 | `Device.Optical.Interface.1.X_RDK_Transceiver.LowerOpticalThreshold` | Lower RX optical power alarm threshold in Dbm1000 units (dBm × 1000). Moved from BBF-deleted standard path. Example: −28000 = −28.0 dBm. | int32 | RO |
| 36 | `Device.Optical.Interface.1.X_RDK_Transceiver.UpperOpticalThreshold` | Upper RX optical power alarm threshold in Dbm1000 units (dBm × 1000). Moved from BBF-deleted standard path. Example: −8000 = −8.0 dBm. | int32 | RO |
| 37 | `Device.Optical.Interface.1.X_RDK_Transceiver.LowerTransmitPowerThreshold` | Lower TX optical power alarm threshold in Dbm1000 units (dBm × 1000). Moved from BBF-deleted standard path. Example: −1000 = −1.0 dBm. | int32 | RO |
| 38 | `Device.Optical.Interface.1.X_RDK_Transceiver.UpperTransmitPowerThreshold` | Upper TX optical power alarm threshold in Dbm1000 units (dBm × 1000). Moved from BBF-deleted standard path. Example: 5000 = 5.0 dBm. | int32 | RO |

---

### 1.6 X\_RDK\_EPON Parameters

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 39 | `Device.Optical.Interface.1.X_RDK_EPON.OperationalMode` | EPON operational mode string, e.g., _1G-EPON_ or _10G-EPON_. | string | RO |
| 40 | `Device.Optical.Interface.1.X_RDK_EPON.EncryptionMode` | Current encryption mode: _Disabled_, _AES-128_, _TripleChurning_, _AES-256_. | string | RO |
| 41 | `Device.Optical.Interface.1.X_RDK_EPON.ONUStatus` | ONU registration status: _LOS_, _DownstreamSignalDetected_, _Registration_, _Deregistration_. | string | RO |
| 42 | `Device.Optical.Interface.1.X_RDK_EPON.DPoESupported` | Whether DPoE (DOCSIS Provisioning of EPON) is supported by the ONU. | boolean | RO |
| 43 | `Device.Optical.Interface.1.X_RDK_EPON.MaxLLIDSupported` | Maximum number of LLIDs supported by the ONU. | uint32 | RO |

---

### 1.7 Manufacturer Information

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 44 | `Device.Optical.Interface.1.X_RDK_EPON.Manufacturer` | ONU manufacturer name. | string | RO |
| 45 | `Device.Optical.Interface.1.X_RDK_EPON.ModelNumber` | ONU model number. | string | RO |
| 46 | `Device.Optical.Interface.1.X_RDK_EPON.HardwareVersion` | ONU hardware revision. | string | RO |
| 47 | `Device.Optical.Interface.1.X_RDK_EPON.SoftwareVersion` | ONU firmware/software version. | string | RO |
| 48 | `Device.Optical.Interface.1.X_RDK_EPON.SerialNumber` | ONU serial number. | string | RO |
| 49 | `Device.Optical.Interface.1.X_RDK_EPON.VendorOUI` | ONU vendor Organizationally Unique Identifier (3 bytes). | string | RO |

---

### 1.8 OLT Information

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 50 | `Device.Optical.Interface.1.X_RDK_EPON.OLT.MACAddress` | OLT MAC address from MPCP GATE messages (IEEE 802.3ah Clause 64.3.3). | string | RO |
| 51 | `Device.Optical.Interface.1.X_RDK_EPON.OLT.VendorOUI` | OLT vendor OUI from OAM Information OAMPDU (Clause 57.4.2.2). | string | RO |
| 52 | `Device.Optical.Interface.1.X_RDK_EPON.OLT.VendorSpecificInfo` | Vendor-specific information from Organization Specific OAMPDU (Clause 57.4.3.3). | string | RO |

---

### 1.9 LLID Dynamic Table

Rows are registered/unregistered dynamically as LLIDs are provisioned by the OLT. Max 64 instances.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 53 | `Device.Optical.Interface.1.X_RDK_EPON.LLIDNumberOfEntries` | Number of active LLID table rows. | uint32 | RO |
| 54 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.` | Dynamic table placeholder for LLID instances. | — | Table |
| 55 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.LLID` | The LLID value assigned by the OLT. | uint32 | RO |
| 56 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.Status` | LLID registration state: _Unregistered_, _Registering_, _Registered_, _Deregistering_, _Failed_. | string | RO |
| 57 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.MACAddress` | MAC address associated with this LLID. | string | RO |
| 58 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.Mode` | LLID mode: _Unicast_ or _Broadcast_. | string | RO |
| 59 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.EncryptionEnabled` | Whether encryption is enabled on this LLID. | boolean | RO |
| 60 | `Device.Optical.Interface.1.X_RDK_EPON.LLID.{i}.ForwardingState` | Forwarding state: _Disabled_, _Enabled_, _Learning_. | string | RO |

---

### 1.10 DPoE / CPE Dynamic Table

Present when DPoE is supported. Max 256 CPE instances.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 61 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.MaxCPECount` | Maximum CPE devices allowed by OLT policy. | uint32 | RO |
| 62 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.StaticCPECount` | Number of statically provisioned CPE entries. | uint32 | RO |
| 63 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.DynamicCPECount` | Number of dynamically learned CPE entries. | uint32 | RO |
| 64 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.CPENumberOfEntries` | Total number of CPE table rows. | uint32 | RO |
| 65 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.CPE.{i}.` | Dynamic table placeholder for CPE instances. | — | Table |
| 66 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.CPE.{i}.MACAddress` | MAC address of the CPE device. | string | RO |
| 67 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.CPE.{i}.Type` | CPE type: _Static_ or _Dynamic_. | string | RO |
| 68 | `Device.Optical.Interface.1.X_RDK_EPON.DPOE.CPE.{i}.AgeTime` | Age time in seconds since the CPE was last seen (0 for static entries). | uint32 | RO |

---

### 1.11 VEIP Interface Dynamic Table

Represents virtual Ethernet interface points. Max 16 instances.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 69 | `Device.Optical.Interface.1.X_RDK_EPON.VEIP_InterfaceNumberOfEntries` | Number of active VEIP interface table rows. | uint32 | RO |
| 70 | `Device.Optical.Interface.1.X_RDK_EPON.VEIP_Interface.{i}.` | Dynamic table placeholder for VEIP instances. | — | Table |
| 71 | `Device.Optical.Interface.1.X_RDK_EPON.VEIP_Interface.{i}.Name` | OS interface name (e.g., _veip0_, _veip1_). | string | RO |
| 72 | `Device.Optical.Interface.1.X_RDK_EPON.VEIP_Interface.{i}.Status` | Interface link status: _Up_ or _Down_. | string | RO |

---

### 1.12 Stats Poller Configuration

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 73 | `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.Enable` | Enable or disable the periodic statistics poller. Persisted in PSM. | boolean | RW |
| 74 | `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.PollingInterval` | Statistics polling interval in seconds (default: 900). Persisted in PSM. | uint32 | RW |

---

### 1.13 ONU Control Actions

Write-triggered boolean parameters. SET to _true_ invokes the corresponding HAL action. GET always returns _false_.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|-----------------|-------------|-----------|--------|
| 75 | `Device.Optical.Interface.1.X_RDK_EPON.Reset` | Triggers an ONU re-registration. Calls `epon_hal_reset_onu()` which deregisters the ONU and restarts the MPCP discovery and OAM negotiation process. GET always returns _false_. | boolean | RW |
| 76 | `Device.Optical.Interface.1.X_RDK_EPON.FactoryReset` | Resets the EPON HAL configuration to factory defaults. Calls `epon_hal_factory_reset()` which clears all custom settings and statistics, and restores default operational parameters. The ONU must be reconfigured and re-initialized after this operation. GET always returns _false_. | boolean | RW |

> **Summary:** 76 total parameters — 7 Read/Write, 63 Read-Only, 3 Table

---

## 2. Telemetry Events

This section defines the complete set of telemetry events, alarms, and status markers reported by the EPON Manager to the RDK T2 telemetry framework. Each event includes a unique marker name, description, priority, and expected Estimated Frequency.

All telemetry markers follow the naming convention: `EPON_<CATEGORY>_<EVENT_NAME>`

---

### 2.1 ONU Status Events

Triggered by the HAL `status_callback` when the ONU registration state changes.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_ONU_LOS` | Physical layer is down. No downstream optical signal detected. The ONU has lost connectivity to the OLT. | **Critical** | On fiber cut or OLT failure. Rare in stable networks; frequent during outages. Typically 0–2 times/day. |
| `EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED` | Downstream optical signal detected but ONU is not yet registered with the OLT. Transitional state during startup or recovery. | **Info** | During startup or after LOS recovery. Typically 1–2 times per boot cycle. Transient — lasts seconds. |
| `EPON_ONU_REGISTRATION` | ONU has completed MPCP discovery and OAM negotiation. LLID-0 is online and the ONU is fully registered with the OLT. | **Info** | On successful registration. Typically once per boot. May recur after LOS recovery (1–3 times/day). |
| `EPON_ONU_DEREGISTRATION` | ONU has been deregistered from the OLT. LLID-0 is offline. May be OLT-initiated or due to link failure. | **Warning** | On OLT-initiated deregistration, MPCP timeout, or link failure. 0–1 times/day normally. |

---

### 2.2 Interface Link Status Events

Triggered by the HAL `interface_status_callback` when a Layer-2 interface (e.g., veip0, veip1) changes state.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_INTF_LINK_UP` | An EPON WAN interface (e.g., veip0) is operationally up. Traffic can flow. Reported per-interface. | **Info** | After ONU registration when OLT provisions the interface. 1–3 times/day. |
| `EPON_INTF_LINK_DOWN` | An EPON WAN interface is operationally down. No traffic. Reported per-interface. | **Warning** | On ONU deregistration, fiber cut, or OLT-side reconfiguration. 0–2 times/day. |
| `EPON_PHY_STATUS_UP` | Aggregate PHY status: at least one interface is UP. WanManager notified EPON WAN is available. | **Info** | When the first interface comes up after all were down. 1–2 times/day. |
| `EPON_PHY_STATUS_DOWN` | Aggregate PHY status: all interfaces are DOWN. WanManager notified EPON WAN is unavailable. | **Critical** | When the last interface goes down. 0–1 times/day; may spike during outages. |

---

### 2.3 Standard IEEE 802.3ah Alarms

Defined by the IEEE 802.3ah OAM specification. Triggered by the HAL `alarm_callback` with `alarm_type = EPON_ALARM_TYPE_STANDARD`.

> **Note:** Each alarm is reported as both **RAISED** (alarm active) and **CLEARED** (alarm resolved). The event value includes the state: `"RAISED"` or `"CLEARED"`.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_ALARM_STD_LOFI` | Loss of Frame/Lock — ONU has lost frame alignment on the downstream signal. Indicates physical layer synchronization failure. _Raised when sync is lost; cleared when sync is restored._ | **Critical** | On physical layer sync loss. Rare under stable conditions. 0–1 times/day. |
| `EPON_ALARM_STD_ERROR_SYMBOL_PERIOD` | Errored Symbol Period threshold exceeded (IEEE 802.3ah Clause 57.5.2). _Raised when threshold exceeded; cleared when error rate drops below threshold._ | **Error** | On degraded link quality. Fiber bend, connector issues, transceiver aging. 0–5 times/day. |
| `EPON_ALARM_STD_ERROR_FRAME` | Errored Frame threshold exceeded (IEEE 802.3ah Clause 57.5.3). _Raised on threshold breach; cleared when frame error rate recovers._ | **Error** | On sustained frame errors caused by physical layer issues. 0–5 times/day. |
| `EPON_ALARM_STD_ERROR_FRAME_PERIOD` | Errored Frame Period threshold exceeded (IEEE 802.3ah Clause 57.5.4). _Raised on threshold breach; cleared when error rate drops._ | **Error** | Measured over a fixed period. 0–5 times/day. |
| `EPON_ALARM_STD_ERROR_FRAME_SECONDS` | Errored Frame Seconds threshold exceeded (IEEE 802.3ah Clause 57.5.5). _Raised on threshold breach; cleared when error-free seconds are restored._ | **Warning** | Sustained error condition. Less frequent. 0–2 times/day. |
| `EPON_ALARM_STD_OAM_SESSION_LOST` | OAM session lost — OAM discovery/keepalive detected loss of OAM peer (OLT). _Raised when session is lost; cleared when OAM rediscovery succeeds._ | **Critical** | On OAM keepalive failure. Precedes/follows deregistration. 0–1 times/day. |
| `EPON_ALARM_STD_EQUIPMENT_FAILURE` | Equipment or hardware failure detected on the ONU. Critical hardware malfunction. _Raised on failure detection; cleared after hardware recovery or replacement._ | **Critical** | Very rare. 0–1 times per device lifetime. |

---

### 2.4 Vendor-Specific Alarms (DPoE)

Vendor-specific extensions. Triggered by the HAL `alarm_callback` with `alarm_type = EPON_ALARM_TYPE_VENDOR_SPECIFIC`.

> **Note:** Each alarm is reported as both **RAISED** (alarm active) and **CLEARED** (alarm resolved). The event value includes the state: `"RAISED"` or `"CLEARED"`.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_ALARM_VENDOR_LOS` | Loss of Signal — no optical signal detected at the receiver. Complete loss of downstream optical power. _Raised when signal is lost; cleared when signal is restored._ | **Critical** | On fiber cut, OLT port failure, or severe attenuation. 0–2 times/day. |
| `EPON_ALARM_VENDOR_DYING_GASP` | Dying Gasp — imminent power loss detected on the ONU. Last message sent before power failure. _Raised on power loss; cleared after power restoration and ONU reboot._ | **Critical** | On power failure or UPS depletion. 0–1 times/month. |
| `EPON_ALARM_VENDOR_POWER_LOW` | Optical receive power below the lower threshold. Signal degraded. May precede LOS. _Raised when power drops; cleared when power recovers._ | **Warning** | Gradual fiber degradation, dirty connectors. 0–3 times/day during degradation. |
| `EPON_ALARM_VENDOR_POWER_HIGH` | Optical receive power exceeds upper threshold. Incorrect fiber connection or splitter misconfiguration. _Raised when power exceeds threshold; cleared when power normalizes._ | **Warning** | Configuration change or fiber reconnection. 0–1 times/month. |
| `EPON_ALARM_VENDOR_TEMPERATURE` | Transceiver module temperature exceeded safe threshold. Risk of hardware damage. _Raised on over-temperature; cleared when temperature returns to safe range._ | **Error** | Environmental overheating. 0–1 times/day in hot environments. |
| `EPON_ALARM_VENDOR_FEC_THRESHOLD` | FEC uncorrectable errors exceeded threshold. High BER indicating imminent link failure. _Raised on threshold breach; cleared when error rate recovers._ | **Error** | Severely degraded link quality. 0–3 times/day during degradation. |
| `EPON_ALARM_VENDOR_LASER_BIAS_CURRENT` | Laser bias current out of normal range. Indicates laser aging or failure. _Raised when out of range; cleared when it returns to normal._ | **Error** | Transceiver degradation. Very rare. 0–1 per device lifetime. |
| `EPON_ALARM_VENDOR_SUPPLY_VOLTAGE` | Supply voltage out of nominal range. Power supply issues. _Raised when out of range; cleared when voltage stabilizes._ | **Error** | Power supply degradation. 0–1 times/month. |

---

### 2.5 System Lifecycle Events

Track the EPON Manager application lifecycle.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_SYSTEM_INIT_SUCCESS` | EPON Manager and HAL initialized successfully. All subsystems (RBus, PSM, HAL, Stats Poller) started. | **Info** | Once per boot or process restart. 1/day normally. |
| `EPON_SYSTEM_INIT_FAILURE` | EPON Manager initialization failed. Includes reason code (HAL failure, RBus failure, wrong PON mode, etc.). | **Critical** | On initialization failure. Should be 0 normally. |
| `EPON_SYSTEM_SHUTDOWN` | EPON Manager received shutdown signal and is performing graceful teardown. | **Info** | On process termination (SIGTERM/SIGINT). 0–1/day. |
| `EPON_SYSTEM_HAL_WRONG_PON_MODE` | HAL reported hardware is configured for a different PON mode (e.g., ITU PON instead of EPON). | **Critical** | On startup when hardware PON mode is misconfigured. |
| `EPON_SYSTEM_FACTORY_RESET` | Factory reset of EPON HAL configuration triggered. All configuration cleared to defaults. | **Warning** | User-initiated. Very rare. 0–1 per device lifetime. |
| `EPON_SYSTEM_ONU_RESET` | ONU reset triggered. ONU will deregister and attempt re-registration with the OLT. | **Warning** | User or OLT-initiated reset. 0–1 times/month. |

---

### 2.6 Error Events

Runtime errors and anomalies. Error markers use the `_accum` suffix so the T2 daemon accumulates all occurrences (up to 20 per reporting cycle) as a JSON array. No application-side rate-limiting is performed.

| Event Name | Meaning | Priority | Estimated Frequency |
|-----------|---------|----------|---------------------|
| `EPON_ERROR_HAL_CALL_FAILED_accum` | A HAL API call returned an error. Includes the API name and return code. | **Error** | On HAL communication or hardware errors. Should be 0 normally. |
| `EPON_ERROR_EVENT_QUEUE_FULL_accum` | Internal event queue is full and an event was dropped. | **Error** | Under extreme event load (flapping link). Very rare. |
| `EPON_ERROR_STATS_COLLECTION_FAILED_accum` | Periodic statistics collection failed. HAL returned an error during stats retrieval. | **Warning** | On intermittent HAL failures. 0–2 times/day. |
| `EPON_ERROR_RBUS_PUBLISH_FAILED_accum` | Failed to publish an event or update a parameter via RBus. | **Error** | On RBus communication failure. Rare. |
| `EPON_ERROR_PSM_ACCESS_FAILED_accum` | Failed to read or write a value from/to the Persistent Storage Manager (PSM). | **Error** | On PSM service issues. Rare. |

---

### 2.7 Priority Definitions

| Priority | Description | Action |
|---------|-------------|--------|
| **Critical** | Service-impacting event. Immediate attention required. Total loss of connectivity or imminent hardware failure. | Raise alarm in monitoring dashboard. Trigger incident workflow. |
| **Error** | Degraded service or component failure. May lead to service impact if not addressed. | Log prominently. Generate alert for network operations. |
| **Warning** | Abnormal condition that may indicate a developing problem. Service still operational. | Log for trend analysis. Investigate if persistent. |
| **Info** | Normal operational event. Used for auditing, state tracking, and troubleshooting. | Log for reference. No action required. |

---

### 2.8 Summary Statistics

| Category | Total Events | Critical | Error | Warning | Info |
|---------|-------------|---------|-------|---------|------|
| ONU Status Events | 4 | 1 | 0 | 1 | 2 |
| Interface Link Events | 4 | 1 | 0 | 1 | 2 |
| Standard Alarms | 7 | 3 | 3 | 1 | 0 |
| Vendor Alarms | 8 | 2 | 4 | 2 | 0 |
| System Lifecycle | 6 | 2 | 0 | 2 | 2 |
| Error Events | 5 | 0 | 3 | 2 | 0 |
| **Total** | **34** | **9** | **10** | **9** | **6** |

> **Note:** Each alarm event carries a raised/cleared state in its value. The priority listed is for the raised condition; cleared events are informational.

---

## References

- BBF TR-181 Issue 2 Amendment 18 — Device Data Model
- IEEE 802.3ah — Ethernet in the First Mile (OAM specification)
- DPoE Specification — DOCSIS Provisioning of EPON
- RDK T2 Telemetry Framework documentation
- Apache Avro 1.11 Specification
- EPON HAL API Header (`epon_hal.h`)
- EPON Manager Design Documents (`design_docs/`)
