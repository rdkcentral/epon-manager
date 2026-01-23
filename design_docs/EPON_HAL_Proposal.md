# EPON HAL Proposal Document

## Executive Summary

This document proposes the EPON (Ethernet Passive Optical Network) Hardware Abstraction Layer (HAL) API for RDK-B platforms. The HAL provides a standardized interface for EPON ONU (Optical Network Unit) management, statistics collection, and monitoring, aligned with TR-181 Device.Optical.Interface data model specifications.

**Version:** 1.0.0  
**Date:** November 21, 2025  
**Target Platform:** RDK-B EPON Devices

---

## 1. TR-181 Data Model Mapping

### 1.1 Device.Optical.Interface.{i} Object

This HAL implementation supports the TR-181 Device.Optical.Interface data model with additional RDK-specific extensions for EPON-specific features.

#### 1.1.1 Status Parameters

| TR-181 Parameter | Type | Source API | Description |
|-----------------|------|------------|-------------|
| `Device.Optical.Interface.{i}.Status` | string | `epon_hal_init()` (callback) | Operational status of the optical interface. Maps from `epon_onu_status_t` enum. |
| `Device.Optical.Interface.{i}.Name` | string | `epon_hal_get_interface_list()` | Interface name (e.g., "veip0"). |
| `Device.Optical.Interface.{i}.LowerLayers` | string | N/A | Lower layer interface reference (not a HAL dependent). |
| `Device.Optical.Interface.{i}.Upstream` | boolean | Constant | (not a HAL dependent). |

**Status Mapping:**
- `EPON_ONU_STATUS_REGISTRATION` → "Up"
- `EPON_ONU_STATUS_DEREGISTRATION` → "Down"
- `EPON_ONU_STATUS_LOS` → "LowerLayerDown"
- `EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED` → "Dormant"

#### 1.1.2 Optical Parameters

| TR-181 Parameter | Type | Source API | Description |
|-----------------|------|------------|-------------|
| `Device.Optical.Interface.{i}.TransmitOpticalLevel` | float | `epon_hal_get_transceiver_stats()` | Current optical transmit power level in dBm. |
| `Device.Optical.Interface.{i}.OpticalSignalLevel` | float | `epon_hal_get_transceiver_stats()` | Received optical power level in dBm. |
| `Device.Optical.Interface.{i}.LowerOpticalThreshold` | float | `epon_hal_get_transceiver_stats()` | Lower receive power threshold in dBm. |
| `Device.Optical.Interface.{i}.UpperOpticalThreshold` | float | `epon_hal_get_transceiver_stats()` | Upper receive power threshold in dBm. |
| `Device.Optical.Interface.{i}.LowerTransmitPowerThreshold` | float | `epon_hal_get_transceiver_stats()` | Lower transmit power threshold in dBm. |
| `Device.Optical.Interface.{i}.UpperTransmitPowerThreshold` | float | `epon_hal_get_transceiver_stats()` | Upper transmit power threshold in dBm. |
| `Device.Optical.Interface.{i}.SFPReferenceList` | string | N/A |(not a HAL dependent) |

### 1.2 Device.Optical.Interface.{i}.Stats Object

| TR-181 Parameter | Type | Source API | Description |
|-----------------|------|------------|-------------|
| `Device.Optical.Interface.{i}.Stats.BytesSent` | uint64 | `epon_hal_get_link_stats()` | Total bytes transmitted. |
| `Device.Optical.Interface.{i}.Stats.BytesReceived` | uint64 | `epon_hal_get_link_stats()` | Total bytes received. |
| `Device.Optical.Interface.{i}.Stats.PacketsSent` | uint64 | `epon_hal_get_link_stats()` | Total packets transmitted. |
| `Device.Optical.Interface.{i}.Stats.PacketsReceived` | uint64 | `epon_hal_get_link_stats()` | Total packets received. |
| `Device.Optical.Interface.{i}.Stats.ErrorsSent` | uint64 | `epon_hal_get_link_stats()` | Errors on transmission. |
| `Device.Optical.Interface.{i}.Stats.ErrorsReceived` | uint64 | `epon_hal_get_link_stats()` | Errors on reception. |
| `Device.Optical.Interface.{i}.Stats.DiscardPacketsSent` | uint64 | `epon_hal_get_link_stats()` | Packets discarded prior to transmission. |
| `Device.Optical.Interface.{i}.Stats.DiscardPacketsReceived` | uint64 | `epon_hal_get_link_stats()` | Packets discarded on reception. |
| `Device.Optical.Interface.{i}.Stats.MaxBitRate` | uint32 | `epon_hal_get_link_stats()` | Maximum bit rate in Mbps. |

### 1.3 RDK Extended Parameters (X_RDK Namespace)

The following parameters extend TR-181 with EPON-specific metrics not covered by the standard specification:

#### 1.3.1 Device.Optical.Interface.{i}.Stats.X_RDK Extensions

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.Stats.X_RDK_FECCorrected` | uint64 | `epon_hal_get_link_stats()` | Number of FEC (Forward Error Correction) corrected bits. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_FECUncorrectable` | uint64 | `epon_hal_get_link_stats()` | Number of FEC uncorrectable codewords. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_BroadcastPacketsSent` | uint64 | `epon_hal_get_link_stats()` | Broadcast packets transmitted. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_BroadcastPacketsReceived` | uint64 | `epon_hal_get_link_stats()` | Broadcast packets received. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_MulticastPacketsSent` | uint64 | `epon_hal_get_link_stats()` | Multicast packets transmitted. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_MulticastPacketsReceived` | uint64 | `epon_hal_get_link_stats()` | Multicast packets received. |

| `Device.Optical.Interface.{i}.Stats.X_RDK_RangingResyncs` | uint64 | `epon_hal_get_link_stats()` | Number of ranging resynchronizations. |
| `Device.Optical.Interface.{i}.Stats.X_RDK_MACResets` | uint64 | `epon_hal_get_link_stats()` | Number of MAC layer resets. |

> **Note:** Clock statistics are under consideration for inclusion. Need to determine what clock-related information is required (e.g., clock accuracy, drift, synchronization status, timing recovery metrics).

#### 1.3.2 Device.Optical.Interface.{i}.X_RDK_Transceiver Object

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_Transceiver.BiasCurrent` | float | `epon_hal_get_transceiver_stats()` | Laser bias current in milliamperes. |
| `Device.Optical.Interface.{i}.X_RDK_Transceiver.Temperature` | float | `epon_hal_get_transceiver_stats()` | Module temperature in Celsius. |
| `Device.Optical.Interface.{i}.X_RDK_Transceiver.SupplyVoltage` | float | `epon_hal_get_transceiver_stats()` | Supply voltage in volts. |

#### 1.3.3 Device.Optical.Interface.{i}.X_RDK_EPON Object

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_EPON.OperationalMode` | string | `epon_hal_get_link_info()` | EPON mode: "1G-EPON" or "10G-EPON". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.EncryptionMode` | string | `epon_hal_get_link_info()` | Encryption mode: "Disabled", "AES-128", "TripleChurning", "AES-256". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.ONUStatus` | string | Callback | Detailed ONU registration status (see mapping below). |
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPoESupported` | boolean | `epon_hal_init()` config | Indicates if DPoE is supported. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.Manufacturer` | string | `epon_hal_get_manufacturer_info()` | Manufacturer name. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.ModelNumber` | string | `epon_hal_get_manufacturer_info()` | Model number. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.HardwareVersion` | string | `epon_hal_get_manufacturer_info()` | Hardware version. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.SoftwareVersion` | string | `epon_hal_get_manufacturer_info()` | Software/firmware version. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.SerialNumber` | string | `epon_hal_get_manufacturer_info()` | Device serial number. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.VendorOUI` | string | `epon_hal_get_manufacturer_info()` | Vendor Organizationally Unique Identifier (3 bytes). |
| `Device.Optical.Interface.{i}.X_RDK_EPON.MaxLLIDSupported` | uint32 | `epon_hal_get_llid_info()` | Maximum LLIDs supported. |

**ONU Status Values:**
- `LOS` - Loss of signal (PHY down)
- `DownstreamSignalDetected` - Downstream signal detected (power present, ONU not yet registered)
- `Registration` - LLID-0 is online (includes MPCP and OAM registration)
- `Deregistration` - LLID-0 is offline

#### 1.3.4 Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}

LLID (Logical Link Identifier) table for multi-LLID support:

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.LLIDValue` | uint16 | `epon_hal_get_llid_info()` | Logical Link Identifier value. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.Mode` | string | `epon_hal_get_llid_info()` | "Unicast" or "Broadcast". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.State` | string | `epon_hal_get_llid_info()` | "Unregistered", "Registering", "Registered", "Deregistering", "Failed". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.ForwardingState` | string | `epon_hal_get_llid_info()` | "Disabled", "Enabled", "Learning". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.EncryptionEnabled` | boolean | `epon_hal_get_llid_info()` | Encryption enabled for this LLID. Encryption type is specified in Device.Optical.Interface.{i}.X_RDK_EPON.EncryptionMode. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i}.LocalMACAddress` | string | `epon_hal_get_llid_info()` | MAC address associated with this LLID. |

#### 1.3.5 Device.Optical.Interface.{i}.X_RDK_EPON.OLT Object

Information about the connected OLT (Optical Line Terminal) learned via MPCP and OAM per IEEE 802.3ah:

| Parameter | Type | Source API | IEEE 802.3ah Reference | Description |
|-----------|------|------------|----------------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_EPON.OLT.MACAddress` | string | `epon_hal_get_olt_info()` | Clause 64.3.3 | OLT MAC address from MPCP GATE messages. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.OLT.VendorOUI` | string | `epon_hal_get_olt_info()` | Clause 57.4.2.2 | OLT vendor OUI from OAM Information OAMPDU. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.OLT.VendorSpecificInfo` | string | `epon_hal_get_olt_info()` | Clause 57.4.3.3 | Vendor-specific information from Organization Specific OAMPDU. |

### 1.4 Device.Optical.Interface.{i}.X_RDK_EPON.DPOE Object

When DPoE support is enabled:

#### 1.4.1 Device.Optical.Interface.{i}.X_RDK_EPON.DPOE Parameters

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.MaxCPECount` | uint32 | `dpoe_hal_get_cpe_mac_table()` | Maximum CPE entries supported. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.StaticCPECount` | uint32 | `dpoe_hal_get_cpe_mac_table()` | Number of static CPE entries. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.DynamicCPECount` | uint32 | `dpoe_hal_get_cpe_mac_table()` | Number of dynamic CPE entries. |

#### 1.4.2 Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.CPE.{i}

| Parameter | Type | Source API | Description |
|-----------|------|------------|-------------|
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.CPE.{i}.MACAddress` | string | `dpoe_hal_get_cpe_mac_table()` | CPE MAC address. |
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.CPE.{i}.Type` | string | `dpoe_hal_get_cpe_mac_table()` | "Static" or "Dynamic". |
| `Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.CPE.{i}.AgeTime` | uint32 | `dpoe_hal_get_cpe_mac_table()` | Age time in seconds (0 for static). |

---

## 2. Telemetry Markers

### 2.1 Recommended Telemetry Metrics

The following telemetry markers are recommended for monitoring EPON ONU health and performance:

#### 2.1.1 Critical Health Metrics (High Priority)

| Telemetry Marker | Source | Threshold/Alert |
|------------------|--------|------------------|
| `EPON_ONU_Status` | Status callback | Alert on LOS, Fault, Timeouts |
| `EPON_Optical_RX_Power` | `epon_hal_get_transceiver_stats()` | Alert if outside thresholds |
| `EPON_Optical_TX_Power` | `epon_hal_get_transceiver_stats()` | Alert if outside thresholds |
| `EPON_FEC_Uncorrectable` | `epon_hal_get_link_stats()` | Alert if increasing rapidly |
| `EPON_Link_Status` | Status callback | Alert on link down |
| `EPON_BER_Errors` | `epon_hal_get_link_stats()` | Alert if exceeds 10^-6 |

#### 2.1.2 Performance Metrics (Medium Priority)

| Telemetry Marker | Source | Purpose |
|------------------|--------|---------|
| `EPON_TX_Bytes` | `epon_hal_get_link_stats()` | Bandwidth monitoring |
| `EPON_RX_Bytes` | `epon_hal_get_link_stats()` | Bandwidth monitoring |
| `EPON_TX_Errors` | `epon_hal_get_link_stats()` | Link quality |
| `EPON_RX_Errors` | `epon_hal_get_link_stats()` | Link quality |
| `EPON_FEC_Corrected` | `epon_hal_get_link_stats()` | FEC efficiency |
| `EPON_Discard_Packets` | `epon_hal_get_link_stats()` | Buffer overflow detection |
| `EPON_Ranging_Resyncs` | `epon_hal_get_link_stats()` | Timing stability |
| `EPON_MAC_Resets` | `epon_hal_get_link_stats()` | MAC layer stability |

#### 2.1.3 OLT Information Metrics (Low Priority)

| Telemetry Marker | Source | Purpose |
|------------------|--------|---------||
| `EPON_OLT_MAC_Address` | `epon_hal_get_olt_info()` | OLT identification |
| `EPON_OLT_Vendor_OUI` | `epon_hal_get_olt_info()` | OLT vendor tracking |

#### 2.1.4 Alarm Events (Event-based)

| Telemetry Event | Source | Priority | Description |
|-----------------|--------|----------|-------------|
| `EPON_ALARM_LOS` | Alarm callback | Critical | Loss of signal detected |
| `EPON_ALARM_DYING_GASP` | Alarm callback | Critical | Imminent power loss |
| `EPON_ALARM_OAM_SESSION_LOST` | Alarm callback | High | OAM session lost |
| `EPON_ALARM_POWER_LOW` | Alarm callback | High | Optical power below threshold |
| `EPON_ALARM_POWER_HIGH` | Alarm callback | High | Optical power above threshold |
| `EPON_ALARM_TEMPERATURE` | Alarm callback | Medium | Temperature threshold exceeded |
| `EPON_ALARM_FEC_THRESHOLD` | Alarm callback | Medium | FEC uncorrectable errors threshold |

#### 2.1.5 Environmental Metrics (Low Priority)

| Telemetry Marker | Source | Threshold/Alert |
|------------------|--------|------------------|
| `EPON_Temperature` | `epon_hal_get_transceiver_stats()` | Alert if > 85°C |
| `EPON_Bias_Current` | `epon_hal_get_transceiver_stats()` | Alert on anomalies |
| `EPON_Supply_Voltage` | `epon_hal_get_transceiver_stats()` | Alert if outside 3.0-3.6V |

---

## 3. API Reference

### 3.1 Initialization and Configuration

#### 3.1.1 `epon_hal_get_version()`
```c
uint32_t epon_hal_get_version(void);
```
Returns the EPON HAL API version for ABI compatibility checking. Applications should verify major version match before using the HAL to ensure compatibility.

#### 3.1.2 `epon_hal_init()`
```c
int epon_hal_init(const epon_hal_config_t *config);
```
Initializes the EPON HAL module with configuration including DPoE support and callback handlers for status changes and alarms. Must be called before any other HAL functions.

### 3.2 Statistics and Monitoring APIs

#### 3.2.1 `epon_hal_get_link_stats()`
```c
int epon_hal_get_link_stats(epon_hal_link_stats_t *stats);
```
Retrieves comprehensive EPON link statistics including packet counts, byte counts, errors, FEC metrics, BER measurements, and MAC layer statistics for performance monitoring.

#### 3.2.2 `epon_hal_get_transceiver_stats()`
```c
int epon_hal_get_transceiver_stats(epon_hal_transceiver_stats_t *stats);
```
Retrieves optical transceiver statistics including transmit/receive power levels, thresholds, laser bias current, module temperature, and supply voltage for optical link health monitoring.

#### 3.2.3 `epon_hal_get_llid_info()`
```c
int epon_hal_get_llid_info(epon_llid_list_t *llid_list);
```
Retrieves Logical Link Identifier (LLID) information for multi-LLID support, including LLID values, modes, states, MAC addresses, and DSCP marking configuration.

#### 3.2.4 `epon_hal_get_manufacturer_info()`
```c
int epon_hal_get_manufacturer_info(epon_onu_manufacturer_info_t *info);
```
Retrieves ONU manufacturer information including manufacturer name, model number, hardware/software versions, serial number, and vendor OUI as per IEEE 802.3ah specifications.

#### 3.2.5 `epon_hal_get_link_info()`
```c
int epon_hal_get_link_info(epon_hal_link_info_t *info);
```
Retrieves current EPON operational information including link mode (1G-EPON or 10G-EPON) and encryption mode negotiated with the OLT.

#### 3.2.6 `epon_hal_get_interface_list()`
```c
int epon_hal_get_interface_list(epon_interface_list_t *if_list);
```
Retrieves list of interfaces (e.g., veip0, veip1) configured by the OLT along with their current operational status (link up/down). Each interface entry contains the interface name and status, used for mapping S1/IP service interfaces to VLANs or service instances and monitoring their link state.

#### 3.2.7 `epon_hal_get_olt_info()`
```c
int epon_hal_get_olt_info(epon_olt_info_t *olt_info);
```
Retrieves information about the OLT (Optical Line Terminal) learned during MPCP registration and OAM discovery. Includes OLT MAC address, vendor information, OAM capabilities, and supported features per IEEE 802.3ah specification.

### 3.3 Control and Management APIs

#### 3.3.1 `epon_hal_clear_stats()`
```c
int epon_hal_clear_stats(void);
```
Resets all EPON link statistics counters to zero including packet counts, byte counts, and FEC error counters. Does not affect optical power measurements.

#### 3.3.2 `epon_hal_reset_onu()`
```c
int epon_hal_reset_onu(void);
```
Performs ONU soft reset and initiates re-registration with the OLT following MPCP discovery and registration procedure. Causes temporary service disruption.

#### 3.3.3 `epon_hal_factory_reset()`
```c
int epon_hal_factory_reset(void);
```
Resets EPON HAL configuration to factory defaults, clearing all custom settings and statistics. Requires re-initialization with `epon_hal_init()` after completion.

#### 3.3.4 `epon_hal_set_oam_log_mask()`
```c
int epon_hal_set_oam_log_mask(uint32_t oam_log_mask);
```
Configures which IEEE 802.3ah OAM message types (Info, Event, Variable Request/Response, Loopback, MPCP messages) should be logged for debugging and troubleshooting purposes.

### 3.4 DPoE Extension APIs

#### 3.4.1 `dpoe_hal_get_sys_descriptor()`
```c
int dpoe_hal_get_sys_descriptor(char *sys_desc, uint32_t desc_len);
```
Retrieves DPoE system descriptor information per DPoE specification, providing system identification and capability information for DOCSIS provisioning.

#### 3.4.2 `dpoe_hal_get_cpe_mac_table()`
```c
int dpoe_hal_get_cpe_mac_table(dpoe_cpe_mac_table_t *cpe_table);
```
Retrieves CPE MAC address table including both statically configured and dynamically learned CPE devices for DPoE multi-CPE support and tracking.

---

## 4. Implementation Notes

### 4.1 Callback Mechanism

The HAL uses two callback functions registered during initialization:

1. **Status Callback**: Invoked when ONU status changes (e.g., link up/down, registration state changes)
2. **Alarm Callback**: Invoked when alarms are raised or cleared (e.g., LOS, power threshold violations, dying gasp)

### 4.2 Logging Integration

The HAL provides a flexible logging framework:
- Define `HAL_LOG_FUNCTION` before including the header to redirect logs to RDK Logger, syslog, or printf
- Use `epon_hal_set_oam_log_mask()` to control verbosity of OAM message logging
- Default implementation has logging disabled (no-op)

### 4.3 Memory Management

For APIs returning arrays (LLID list, interface list, CPE table):
- Caller may pre-allocate memory, or
- Implementation may allocate internally (implementation-specific)
- Documentation should clarify ownership and free responsibilities

### 4.4 Thread Safety

Implementation should consider:
- Thread-safe access to statistics counters
- Callback invocation context (separate thread or signal handler)
- Mutex protection for shared resources

---

## 5. Benefits and Use Cases

### 5.1 Benefits

1. **TR-181 Compliance**: Aligns with Broadband Forum standards for optical interfaces
2. **Comprehensive Monitoring**: Provides detailed statistics for link health, performance, and diagnostics
3. **Alarm Management**: Real-time notification of critical events and faults
4. **DPoE Support**: Enables DOCSIS provisioning for EPON networks
5. **Vendor Independence**: Standardized API across different EPON chipset vendors
6. **Telemetry Ready**: Rich metrics for cloud-based monitoring and analytics

### 5.2 Use Cases

1. **Service Activation**: Monitor ONU registration progress and link establishment
2. **Performance Monitoring**: Track bandwidth utilization, errors, and FEC statistics
3. **Fault Detection**: Real-time alarm notifications for proactive issue resolution
4. **Capacity Planning**: Historical statistics for network planning
5. **Troubleshooting**: Optical power levels, BER, and OAM message logging for diagnostics
6. **Multi-Service Support**: LLID management for multiple service instances

---

## 6. Testing and Validation

### 6.1 Functional Testing

- [ ] Verify all APIs return correct data
- [ ] Validate callback invocation for status changes and alarms
- [ ] Test statistics accuracy against hardware counters
- [ ] Verify ONU reset and factory reset operations
- [ ] Validate DPoE APIs if supported

### 6.2 Performance Testing

- [ ] Measure API call latency
- [ ] Verify statistics polling does not impact data plane performance
- [ ] Test callback overhead and timing

### 6.3 Stress Testing

- [ ] Rapid polling of statistics
- [ ] Frequent status changes
- [ ] High alarm rate scenarios
- [ ] Long-duration stability testing

### 6.4 Compliance Testing

- [ ] TR-181 data model mapping verification
- [ ] IEEE 802.3ah EPON compliance
- [ ] DPoE specification compliance (if applicable)


---

## 7. Appendix

### 7.1 Error Code Reference

| Return Code | Value | Description |
|-------------|-------|-------------|
| `EPON_HAL_SUCCESS` | 0 | Operation completed successfully |
| `EPON_HAL_ERROR_INVALID_PARAM` | -1 | Invalid parameter provided |
| `EPON_HAL_ERROR_NOT_INITIALIZED` | -2 | HAL not initialized |
| `EPON_HAL_ERROR_HW_FAILURE` | -3 | Hardware operation failed |
| `EPON_HAL_ERROR_NOT_SUPPORTED` | -4 | Operation not supported |
| `EPON_HAL_ERROR_TIMEOUT` | -5 | Operation timeout |
| `EPON_HAL_ERROR_MEMORY` | -6 | Memory allocation failed |
| `EPON_HAL_ERROR_RESOURCE` | -7 | Resource unavailable |
| `EPON_HAL_ERROR_CALLBACK_REG` | -8 | Callback registration failed |
| `EPON_HAL_ERROR_CONFIG` | -9 | Configuration error |
| `EPON_HAL_ERROR` | -10 | General error |

### 7.2 Alarm Types Reference

Complete list of 20 alarm types supported by the HAL, covering optical link faults, OAM events, environmental conditions, and hardware failures.

### 7.3 References

- **TR-181 Issue 2 Amendment 20**: Device Data Model for TR-069
- **IEEE 802.3ah-2004**: Ethernet in the First Mile (EPON)
- **DPoE Specification**: DOCSIS Provisioning of EPON
- **RDK-B Architecture**: Reference Design Kit for Broadband devices

---


**Document Version:** 1.0  
**Last Updated:** November 21, 2025  
**Contact:** [Your contact information]
