# EPON HAL Validation Test Suite

## Overview
This directory contains a comprehensive validation test suite for the EPON HAL library. The test suite includes 33 test cases covering all EPON HAL APIs, including normal operation, error handling, and destructive operations.

## Build Instructions

1. **Configure the build** (ensure RBUS is enabled):
   ```bash
   cd /home/PSJ02/xF10/200226/build-scxf10/tmp/work/cortexa53-rdk-linux/rdk-eponmanager/1.0-r0/git
   autoreconf -fi
   ./configure --enable-rbus
   ```

2. **Build the project**:
   ```bash
   make clean
   make
   ```

3. **Install** (if needed):
   ```bash
   make install
   ```

## Running the HAL Test

### Prerequisites
- EPON Manager must be running (`epon_manager` process)
- RBUS daemon must be active
- The device must be an actual EPON ONU hardware
- Appropriate permissions to invoke RBUS methods

### Triggering the Test via RBUS

#### Method 1: Basic Test (Safe, Non-Destructive)
Run all safe test cases (TC-01 through TC-31):
```bash
rbusmethod_invoke epon_manager Device.Optical.Interface.1.X_RDK_EPON.RunHALTest()
```

#### Method 2: Full Test (Including Destructive Tests)
⚠️ **WARNING**: This includes ONU reset and factory reset operations!
```bash
rbusmethod_invoke epon_manager Device.Optical.Interface.1.X_RDK_EPON.RunHALTest() IncludeDestructive:bool:true
```

### Expected Output

The RBUS method will return:
```json
{
  "TotalTests": 33,
  "Passed": <number>,
  "Failed": <number>,
  "Skipped": <number>,
  "Warnings": <number>,
  "ReportFile": "/rdklogs/logs/epon_hal_test_report.txt",
  "Result": "SUCCESS"
}
```

### Viewing Test Results

#### 1. Check RBUS Response
The immediate output from `rbusmethod_invoke` shows summary statistics.

#### 2. View Detailed Report File
```bash
cat /rdklogs/logs/epon_hal_test_report.txt
```

#### 3. View RDK Logs
All test execution details are logged to RDK Logger:
```bash
tail -f /rdklogs/logs/EPONMANAGERLog.txt.0
# or
grep "HAL-TEST" /rdklogs/logs/EPONMANAGERLog.txt.0
```

## Test Cases

### Non-Destructive Tests (TC-01 to TC-31)
| Test Case | Description | API Tested |
|-----------|-------------|------------|
| TC-01 | HAL Version Check | `epon_hal_get_version()` |
| TC-02 | Link Stats Normal | `epon_hal_get_link_stats()` |
| TC-03 | Link Stats NULL Check | `epon_hal_get_link_stats()` |
| TC-04 | Link Stats Bad Struct Size | `epon_hal_get_link_stats()` |
| TC-05 | Transceiver Stats Normal | `epon_hal_get_transceiver_stats()` |
| TC-06 | Transceiver Stats NULL Check | `epon_hal_get_transceiver_stats()` |
| TC-07 | LLID Info Normal | `epon_hal_get_llid_info()` |
| TC-08 | LLID Info NULL Check | `epon_hal_get_llid_info()` |
| TC-09 | Manufacturer Info Normal | `epon_hal_get_manufacturer_info()` |
| TC-10 | Manufacturer Info NULL Check | `epon_hal_get_manufacturer_info()` |
| TC-11 | Link Info Normal | `epon_hal_get_link_info()` |
| TC-12 | Link Info NULL Check | `epon_hal_get_link_info()` |
| TC-13 | Interface List Normal | `epon_hal_get_interface_list()` |
| TC-14 | Interface List NULL Check | `epon_hal_get_interface_list()` |
| TC-15 | OLT Info Normal | `epon_hal_get_olt_info()` |
| TC-16 | OLT Info NULL Check | `epon_hal_get_olt_info()` |
| TC-17 | Set OAM Log Mask (ALL) | `epon_hal_set_oam_log_mask()` |
| TC-18 | Set OAM Log Mask (NONE) | `epon_hal_set_oam_log_mask()` |
| TC-19 | Set OAM Log Mask (Selective) | `epon_hal_set_oam_log_mask()` |
| TC-20 | Clear Stats & Verify | `epon_hal_clear_stats()` |
| TC-21 | CPE MAC Table Normal | `dpoe_hal_get_cpe_mac_table()` |
| TC-22 | CPE MAC Table NULL Check | `dpoe_hal_get_cpe_mac_table()` |
| TC-23 | Cross-Validate Link Stats | Data layer vs HAL |
| TC-24 | Cross-Validate Transceiver Stats | Data layer vs HAL |
| TC-25 | Cross-Validate Manufacturer Info | Data layer vs HAL |
| TC-26 | Cross-Validate Link Info | Data layer vs HAL |
| TC-27 | Cross-Validate OLT Info | Data layer vs HAL |
| TC-28 | Stability Check | Consecutive calls |
| TC-29 | OLT Info Bad Struct Size | `epon_hal_get_olt_info()` |
| TC-30 | Manufacturer Info Bad Struct Size | `epon_hal_get_manufacturer_info()` |
| TC-31 | Transceiver Stats Bad Struct Size | `epon_hal_get_transceiver_stats()` |

### Destructive Tests (TC-32 to TC-33)
⚠️ **Only run these with `IncludeDestructive:bool:true`**

| Test Case | Description | API Tested | Impact |
|-----------|-------------|------------|--------|
| TC-32 | ONU Reset | `epon_hal_reset_onu()` | Resets ONU, re-registration required |
| TC-33 | Factory Reset | `epon_hal_factory_reset()` | Complete factory reset |

## Test Result Interpretation

### Test Statuses
- **PASS**: Test completed successfully
- **FAIL**: Test failed (API error or validation failure)
- **WARN**: Test passed but with warnings (e.g., unexpected values)
- **SKIP**: Test was skipped (e.g., destructive test without flag)

### Sample Report
```
================================================================================
                    EPON HAL Validation Test Report
================================================================================
Test Execution Date: 2026-02-24 HH:MM:SS
Target Hardware: Actual EPON ONU

Summary
--------------------------------------------------------------------------------
Total Tests:      33
Passed:           31
Failed:           0
Skipped:          2
Warnings:         0

--------------------------------------------------------------------------------
Test Case Details
--------------------------------------------------------------------------------

[1] TC-01 epon_hal_get_version
    Status:  PASS
    Details: Version=1.0.0 Major=1
    Method:  epon_hal_get_version()
    Time:    5 ms

[2] TC-02 epon_hal_get_link_stats
    Status:  PASS
    Details: Bytes RX=12345678 TX=23456789 Frames RX=1234 TX=2345
    Method:  epon_hal_get_link_stats()
    Time:    3 ms

...

[32] TC-32 epon_hal_reset_onu
     Status:  SKIP
     Details: Destructive test - skipped (use IncludeDestructive=true to run)
     Method:  epon_hal_reset_onu()
     Time:    0 ms

[33] TC-33 epon_hal_factory_reset
     Status:  SKIP
     Details: Destructive test - skipped (use IncludeDestructive=true to run)
     Method:  epon_hal_factory_reset()
     Time:    0 ms

================================================================================
```

## Troubleshooting

### "undefined reference to eponMgr_hal_test_rbus_handler"
Rebuild the project:
```bash
make clean
autoreconf -fi
./configure --enable-rbus
make
```

### "Method not found" error
Ensure EPON Manager is running with RBUS enabled:
```bash
ps aux | grep epon_manager
```

### No test report generated
Check permissions on `/rdklogs/logs/` directory and verify EPON Manager has write access.

### All tests show SKIP
Ensure EPON Manager was built with `--enable-rbus` flag.

## Development

### Adding New Test Cases
1. Increment `HAL_TEST_MAX_CASES` in `eponMgr_hal_test.h` if needed
2. Create test function in `eponMgr_hal_test.c` following pattern:
   ```c
   static void test_my_new_case(hal_test_report_t *rpt) {
       uint64_t start = now_ms();
       // Test implementation
       add_result(rpt, "TC-XX test_name", status, detail, method, elapsed);
   }
   ```
3. Call from `eponMgr_hal_test_run_all()`
4. Update this README with new test case

### Files
- `eponMgr_hal_test.h` - Public API and data structures
- `eponMgr_hal_test.c` - Test implementation
- `Makefile.am` - Build configuration
- `README.md` - This file

## License
See repository LICENSE file.
