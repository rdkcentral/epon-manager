# Phase 7: TR-181 Parameter Mapping

**Date:** December 26, 2024  
**Status:** 🔄 In Progress

## Overview

This document maps TR-181 Device.Optical.Interface parameters to EPON HAL wrapper APIs and internal data structures. Each parameter includes:
- TR-181 path
- Data type
- Source (HAL wrapper function or data structure)
- Access mode (GET/SET)
- Implementation notes

---

## 1. Device.Optical.Interface.{i} - Base Parameters

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `Enable` | boolean | GET/SET | N/A (Config) | Read/write from config file via `eponMgr_config` |
| `Status` | string | GET | `eponMgr_onu_state_get_status()` | Map from `epon_onu_status_t` enum:<br>• REGISTRATION → "Up"<br>• DEREGISTRATION → "Down"<br>• LOS → "LowerLayerDown"<br>• DOWNSTREAM_SIGNAL_DETECTED → "Dormant" |
| `Alias` | string | GET/SET | N/A (Config) | User-assigned name, stored in config |
| `Name` | string | GET | `eponMgr_get_interface_name()` | Returns "veip0" or interface name from interface list |
| `LastChange` | uint32 | GET | N/A (Internal) | Timestamp of last status change (Phase 8) |
| `LowerLayers` | string | GET | N/A | Returns empty (no lower layer for optical interface) |
| `Upstream` | boolean | GET | Constant | Returns `false` (EPON is downstream) |

---

## 2. Device.Optical.Interface.{i} - Optical Parameters

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `OpticalSignalLevel` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `rx_power` | Received optical power in 0.1 dBm units |
| `LowerOpticalThreshold` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `rx_power_low_threshold` | Lower RX power threshold in 0.1 dBm |
| `UpperOpticalThreshold` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `rx_power_high_threshold` | Upper RX power threshold in 0.1 dBm |
| `TransmitOpticalLevel` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `tx_power` | Transmit optical power in 0.1 dBm units |
| `LowerTransmitPowerThreshold` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `tx_power_low_threshold` | Lower TX power threshold in 0.1 dBm |
| `UpperTransmitPowerThreshold` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `tx_power_high_threshold` | Upper TX power threshold in 0.1 dBm |

---

## 3. Device.Optical.Interface.{i}.Stats - Statistics

### 3.1 Standard TR-181 Stats

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `BytesSent` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_bytes` | Total bytes transmitted |
| `BytesReceived` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_bytes` | Total bytes received |
| `PacketsSent` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_packets` | Total packets transmitted |
| `PacketsReceived` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_packets` | Total packets received |
| `ErrorsSent` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_errors` | Transmission errors |
| `ErrorsReceived` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_errors` | Reception errors |
| `UnicastPacketsSent` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_unicast_packets` | Unicast packets sent |
| `UnicastPacketsReceived` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_unicast_packets` | Unicast packets received |
| `DiscardPacketsSent` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_discard_packets` | Packets discarded before TX |
| `DiscardPacketsReceived` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_discard_packets` | Packets discarded on RX |
| `MulticastPacketsSent` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_multicast_packets` | Multicast packets sent |
| `MulticastPacketsReceived` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_multicast_packets` | Multicast packets received |
| `BroadcastPacketsSent` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `tx_broadcast_packets` | Broadcast packets sent |
| `BroadcastPacketsReceived` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `rx_broadcast_packets` | Broadcast packets received |
| `UnknownProtoPacketsReceived` | uint32 | GET | N/A | Not available from HAL (return 0) |

### 3.2 X_RDK Extended Stats

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `X_RDK_FECCorrected` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `fec_corrected_bits` | FEC corrected bits |
| `X_RDK_FECUncorrectable` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `fec_uncorrectable_codewords` | FEC uncorrectable codewords |
| `X_RDK_BER` | uint64 | GET | `eponMgr_hal_get_link_stats()`<br>→ `ber_current` | Current bit error rate (x 10^-12) |
| `X_RDK_RangingResyncs` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `ranging_resyncs` | Number of ranging resyncs |
| `X_RDK_MACResets` | uint32 | GET | `eponMgr_hal_get_link_stats()`<br>→ `mac_resets` | Number of MAC resets |

---

## 4. Device.Optical.Interface.{i}.X_RDK_Transceiver

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `Temperature` | int | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `temperature` | Temperature in Celsius |
| `SupplyVoltage` | uint32 | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `supply_voltage` | Supply voltage in millivolts |
| `BiasCurrent` | uint32 | GET | `eponMgr_hal_get_transceiver_stats()`<br>→ `bias_current` | Laser bias current in microamperes |

---

## 5. Device.Optical.Interface.{i}.X_RDK_EPON - EPON Specific

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `OperationalMode` | string | GET | `eponMgr_hal_get_link_info()`<br>→ `epon_mode` | Map from enum:<br>• EPON_MODE_1G → "1G-EPON"<br>• EPON_MODE_10G → "10G-EPON"<br>• EPON_MODE_SYMMETRIC_10G → "10G-EPON-Symmetric" |
| `EncryptionMode` | string | GET | `eponMgr_hal_get_link_info()`<br>→ `encryption_mode` | Map from enum:<br>• DISABLED → "Disabled"<br>• AES_128 → "AES-128"<br>• TRIPLE_CHURNING → "TripleChurning"<br>• AES_256 → "AES-256" |
| `ONUStatus` | string | GET | `eponMgr_onu_state_get_status()` | Map from `epon_onu_status_t`:<br>• LOS → "LOS"<br>• DOWNSTREAM_SIGNAL_DETECTED → "DownstreamSignalDetected"<br>• REGISTRATION → "Registration"<br>• DEREGISTRATION → "Deregistration" |
| `DPoESupported` | boolean | GET | `eponMgr_config_get_dpoe_enabled()` | From initialization config |
| `MaxLLIDSupported` | uint32 | GET | `eponMgr_hal_get_llid_count()` | Returns max LLID count (typically 8 or 32) |

---

## 6. Device.Optical.Interface.{i}.X_RDK_EPON - Manufacturer Info

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `Manufacturer` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `manufacturer` | ONU manufacturer name |
| `ModelNumber` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `model_number` | ONU model number |
| `HardwareVersion` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `hw_version` | Hardware version string |
| `SoftwareVersion` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `sw_version` | Software/firmware version |
| `SerialNumber` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `serial_number` | Device serial number |
| `VendorOUI` | string | GET | `eponMgr_onu_state_get_manufacturer_info()`<br>→ `vendor_oui` | 3-byte OUI (format: "XX:XX:XX") |

---

## 7. Device.Optical.Interface.{i}.X_RDK_EPON.OLT - OLT Information

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `MACAddress` | string | GET | `eponMgr_onu_state_get_olt_info()`<br>→ `mac_address` | OLT MAC from MPCP GATE messages |
| `VendorOUI` | string | GET | `eponMgr_onu_state_get_olt_info()`<br>→ `vendor_oui` | OLT vendor OUI from OAM |
| `VendorSpecificInfo` | string | GET | `eponMgr_onu_state_get_olt_info()`<br>→ `vendor_specific_info` | Vendor-specific data (hex string) |

---

## 8. Device.Optical.Interface.{i}.X_RDK_EPON.LLID.{i} - LLID Table

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `LLIDValue` | uint32 | GET | `eponMgr_llid_list_get_all()`<br>→ `llid` | Logical Link Identifier (0-32767) |
| `Mode` | string | GET | `eponMgr_llid_list_get_all()`<br>→ `mode` | Map from enum:<br>• UNICAST → "Unicast"<br>• BROADCAST → "Broadcast" |
| `State` | string | GET | `eponMgr_llid_list_get_all()`<br>→ `state` | Map from enum:<br>• UNREGISTERED → "Unregistered"<br>• REGISTERING → "Registering"<br>• REGISTERED → "Registered"<br>• DEREGISTERING → "Deregistering"<br>• FAILED → "Failed" |
| `ForwardingState` | string | GET | `eponMgr_llid_list_get_all()`<br>→ `forwarding_state` | Map from enum:<br>• DISABLED → "Disabled"<br>• LEARNING → "Learning"<br>• FORWARDING → "Forwarding" |
| `EncryptionEnabled` | boolean | GET | `eponMgr_llid_list_get_all()`<br>→ `encryption_enabled` | Per-LLID encryption status |
| `LocalMACAddress` | string | GET | `eponMgr_llid_list_get_all()`<br>→ `mac_address` | MAC address for this LLID |

**Note:** LLID.{i} is a **dynamic table**. Use `eponMgr_llid_list_get_count()` to get number of entries.

---

## 9. Device.Optical.Interface.{i}.X_RDK_EPON.DPOE - DPoE Support

### 9.1 DPoE Statistics

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `MaxCPECount` | uint32 | GET | `eponMgr_cpe_list_get_max_count()` | Maximum CPE entries (typically 256) |
| `StaticCPECount` | uint32 | GET | `eponMgr_cpe_list_get_static_count()` | Number of static CPE entries |
| `DynamicCPECount` | uint32 | GET | `eponMgr_cpe_list_get_dynamic_count()` | Number of learned CPE entries |

### 9.2 Device.Optical.Interface.{i}.X_RDK_EPON.DPOE.CPE.{i} - CPE Table

| TR-181 Parameter | Type | Access | HAL Wrapper Source | Implementation |
|------------------|------|--------|-------------------|----------------|
| `MACAddress` | string | GET | `eponMgr_cpe_list_get_all()`<br>→ `mac_address` | CPE MAC address |
| `Type` | string | GET | `eponMgr_cpe_list_get_all()`<br>→ `is_static` | "Static" if true, "Dynamic" if false |
| `AgeTime` | uint32 | GET | `eponMgr_cpe_list_get_all()`<br>→ `age_time` | Age time in seconds (0 for static) |

**Note:** CPE.{i} is a **dynamic table**. Use `eponMgr_cpe_list_get_count()` to get total entries.

---

## 10. Implementation Summary

### 10.1 HAL Wrapper Functions Used

| HAL Wrapper Function | Purpose | TR-181 Parameters Served |
|---------------------|---------|--------------------------|
| `eponMgr_hal_get_link_stats()` | Link statistics | All Stats.* parameters (15+ parameters) |
| `eponMgr_hal_get_transceiver_stats()` | Optical transceiver | OpticalSignalLevel, TransmitOpticalLevel, thresholds, X_RDK_Transceiver.* |
| `eponMgr_hal_get_link_info()` | Link configuration | OperationalMode, EncryptionMode |
| `eponMgr_hal_get_llid_count()` | LLID count | MaxLLIDSupported |
| `eponMgr_onu_state_get_status()` | ONU status | Status, ONUStatus |
| `eponMgr_onu_state_get_manufacturer_info()` | Manufacturer info | Manufacturer, ModelNumber, HardwareVersion, etc. (6 params) |
| `eponMgr_onu_state_get_olt_info()` | OLT information | OLT.MACAddress, OLT.VendorOUI, OLT.VendorSpecificInfo |
| `eponMgr_interface_list_get_name()` | Interface name | Name |
| `eponMgr_llid_list_get_all()` | LLID table | LLID.{i}.* (6 parameters per LLID) |
| `eponMgr_cpe_list_get_all()` | CPE MAC table | DPOE.CPE.{i}.* (3 parameters per CPE) |
| `eponMgr_config_*` | Configuration | Enable, Alias, DPoESupported |

### 10.2 Data Structure Integration

```
TR-181 Parameters
       ↓
eponMgr_rbus (Phase 7)
       ↓
eponMgr_hal_wrapper (with cache)
       ↓
epon_hal (vendor HAL)
```

**Cache Strategy:**
- **Statistics (GET)**: 30-second TTL, refresh on expire
- **Info (GET)**: Validity flags, refresh on status change
- **Config (GET/SET)**: Direct config file access, no cache

### 10.3 TR-181 Parameter Count

| Category | Count | Notes |
|----------|-------|-------|
| Base Interface Parameters | 7 | Enable, Status, Name, etc. |
| Optical Parameters | 6 | Power levels and thresholds |
| Standard Stats | 15 | BytesSent, PacketsReceived, etc. |
| X_RDK Stats | 5 | FEC, BER, ranging, MAC resets |
| X_RDK_Transceiver | 3 | Temperature, voltage, current |
| X_RDK_EPON | 5 | Mode, encryption, status, DPoE |
| Manufacturer Info | 6 | Manufacturer, model, versions |
| OLT Info | 3 | MAC, OUI, vendor info |
| LLID.{i} (per entry) | 6 | × number of LLIDs (1-32) |
| DPOE Stats | 3 | CPE counts |
| DPOE.CPE.{i} (per entry) | 3 | × number of CPEs (0-256) |
| **Total Base Parameters** | **53** | Without dynamic tables |
| **Total with Tables** | **53 + (6 × LLIDs) + (3 × CPEs)** | Typical: ~130 parameters |

---

## 11. Phase 7 Implementation Plan

### Step 1: Create TR-181 Handler Structure
```c
// src/rbus/tr181/eponMgr_tr181.c
// - Parameter registration
// - GET/SET handler dispatch
// - Type conversion (HAL → RBUS)
```

### Step 2: Implement GET Handlers by Category
1. **Base parameters** (Enable, Status, Name)
2. **Optical parameters** (power levels, thresholds)
3. **Statistics** (link stats, FEC, BER)
4. **Transceiver** (temperature, voltage, current)
5. **EPON specific** (mode, encryption, LLID count)
6. **Manufacturer info** (vendor data)
7. **OLT info** (OLT MAC, OUI)
8. **LLID table** (dynamic multi-instance)
9. **DPoE/CPE table** (dynamic multi-instance)

### Step 3: Implement SET Handlers
Only 2 writable parameters:
- `Enable` - Update config file
- `Alias` - Update config file

### Step 4: Register with RBUS
```c
rbusDataElement_t elements[] = {
    {"Device.Optical.Interface.1.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, setHandler, ...}},
    {"Device.Optical.Interface.1.Status", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, ...}},
    // ... all parameters
};
```

### Step 5: Testing
- Unit tests for each handler
- Integration test with HAL mock
- Performance testing (GET latency)
- Cache validation

---

## 12. Next Steps

1. ✅ **Map TR-181 to HAL wrapper** (this document)
2. ⏳ Create tr181 handler structure
3. ⏳ Implement GET handlers (categories 1-9)
4. ⏳ Implement SET handlers (Enable, Alias)
5. ⏳ Register parameters with RBUS
6. ⏳ Create unit tests
7. ⏳ Integration testing
8. ⏳ Update documentation

**Estimated Effort:** 6-8 hours

---

**Document Version:** 1.0  
**Last Updated:** December 26, 2024  
**Status:** Ready for implementation
