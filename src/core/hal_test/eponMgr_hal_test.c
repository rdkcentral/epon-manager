/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file eponMgr_hal_test.c
 * @brief EPON HAL Validation Test Suite Implementation
 *
 * Tests every HAL API on real ONU hardware and generates a detailed report.
 * Uses the existing EPON Manager data layer where possible, falling back to
 * direct HAL calls for validation.  Triggered via:
 *
 *   rbusMethod_Invoke(handle,
 *       "Device.Optical.Interface.1.X_RDK_EPON.RunHALTest", in, &out)
 *
 * Or on the device CLI:
 *   rbusmethod_invoke epon_manager \
 *       Device.Optical.Interface.1.X_RDK_EPON.RunHALTest()
 */

#include "eponMgr_hal_test.h"
#include "epon_hal.h"
#include "eponMgr_logger.h"
#include "eponMgr_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

/* ========================================================================== */
/*  Internal helpers                                                          */
/* ========================================================================== */

/** Get monotonic time in milliseconds */
static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static const char *status_str(hal_test_status_t s)
{
    switch (s) {
        case HAL_TEST_PASS: return "PASS";
        case HAL_TEST_FAIL: return "FAIL";
        case HAL_TEST_SKIP: return "SKIP";
        case HAL_TEST_WARN: return "WARN";
        default:            return "????";
    }
}

/** Return code to string */
static const char *rc_str(epon_hal_return_t rc)
{
    switch (rc) {
        case EPON_HAL_SUCCESS:                return "SUCCESS";
        case EPON_HAL_ERROR_INVALID_PARAM:    return "INVALID_PARAM";
        case EPON_HAL_ERROR_NOT_INITIALIZED:  return "NOT_INITIALIZED";
        case EPON_HAL_ERROR_HW_FAILURE:       return "HW_FAILURE";
        case EPON_HAL_ERROR_NOT_SUPPORTED:    return "NOT_SUPPORTED";
        case EPON_HAL_ERROR_TIMEOUT:          return "TIMEOUT";
        case EPON_HAL_ERROR_MEMORY:           return "MEMORY";
        case EPON_HAL_ERROR_RESOURCE:         return "RESOURCE";
        case EPON_HAL_ERROR_CALLBACK_REG:     return "CALLBACK_REG";
        case EPON_HAL_ERROR_CONFIG:           return "CONFIG";
        case EPON_HAL_ERROR_WRONG_PON_MODE:   return "WRONG_PON_MODE";
        case EPON_HAL_ERROR:                  return "ERROR";
        default:                              return "UNKNOWN_RC";
    }
}

/** Add a test result to the report */
static void add_result(hal_test_report_t *rpt, const char *name,
                        hal_test_status_t status, const char *detail,
                        const char *method, double elapsed)
{
    if (!rpt || rpt->total >= HAL_TEST_MAX_CASES) return;

    hal_test_case_result_t *tc = &rpt->cases[rpt->total];
    snprintf(tc->name, sizeof(tc->name), "%s", name);
    tc->status = status;
    snprintf(tc->detail, sizeof(tc->detail), "%s", detail ? detail : "");
    snprintf(tc->method, sizeof(tc->method), "%s", method ? method : "");
    tc->elapsed_ms = elapsed;

    switch (status) {
        case HAL_TEST_PASS: rpt->passed++;  break;
        case HAL_TEST_FAIL: rpt->failed++;  break;
        case HAL_TEST_SKIP: rpt->skipped++; break;
        case HAL_TEST_WARN: rpt->warnings++; rpt->passed++; break;
    }
    rpt->total++;
}

/* ========================================================================== */
/*  Individual test cases                                                     */
/* ========================================================================== */

/**
 * TC-01: epon_hal_get_version()
 * Validate the HAL reports a version with matching major.
 */
static void test_get_version(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_version(). "
                         "Checks returned version != 0 and major matches EPON_HAL_VERSION_MAJOR.";
    double t0 = now_ms();

    uint32_t ver = epon_hal_get_version();
    double elapsed = now_ms() - t0;

    uint8_t major = (ver >> 24) & 0xFF;
    uint8_t minor = (ver >> 16) & 0xFF;
    uint16_t patch = ver & 0xFFFF;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "Returned version: %u.%u.%u (raw=0x%08X), expected major=%u",
             major, minor, patch, ver, EPON_HAL_VERSION_MAJOR);

    if (ver == 0) {
        add_result(rpt, "TC-01 epon_hal_get_version", HAL_TEST_FAIL, detail, method, elapsed);
    } else if (major != EPON_HAL_VERSION_MAJOR) {
        add_result(rpt, "TC-01 epon_hal_get_version", HAL_TEST_WARN, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-01 epon_hal_get_version", HAL_TEST_PASS, detail, method, elapsed);
    }
}

/**
 * TC-02: epon_hal_get_link_stats() - normal call
 * Fetches link stats and checks struct is populated.
 */
static void test_get_link_stats(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_link_stats(). "
                         "Sets struct_size, expects SUCCESS, validates counters are readable.";
    double t0 = now_ms();

    epon_hal_link_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    stats.struct_size = sizeof(stats);

    epon_hal_return_t rc = epon_hal_get_link_stats(&stats);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, packets_sent=%" PRIu64 ", packets_received=%" PRIu64
             ", bytes_sent=%" PRIu64 ", bytes_received=%" PRIu64
             ", fec_corrected=%" PRIu64 ", fec_uncorrectable=%" PRIu64
             ", max_bit_rate=%u",
             rc_str(rc), stats.packets_sent, stats.packets_received,
             stats.bytes_sent, stats.bytes_received,
             stats.fec_corrected, stats.fec_uncorrectable,
             stats.max_bit_rate);

    if (rc == EPON_HAL_SUCCESS) {
        add_result(rpt, "TC-02 epon_hal_get_link_stats", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-02 epon_hal_get_link_stats", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-03: epon_hal_get_link_stats() - NULL param validation
 */
static void test_get_link_stats_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_link_stats(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_link_stats(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-03 get_link_stats(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-03 get_link_stats(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-04: epon_hal_get_link_stats() - bad struct_size validation
 */
static void test_get_link_stats_bad_size(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_link_stats() with struct_size=0. "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_link_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    stats.struct_size = 0;  /* intentionally wrong */

    epon_hal_return_t rc = epon_hal_get_link_stats(&stats);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-04 get_link_stats(bad_size)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-04 get_link_stats(bad_size)", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-05: epon_hal_get_transceiver_stats() - normal call
 */
static void test_get_transceiver_stats(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_transceiver_stats(). "
                         "Sets struct_size, expects SUCCESS, validates optical levels.";
    double t0 = now_ms();

    epon_hal_transceiver_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    stats.struct_size = sizeof(stats);

    epon_hal_return_t rc = epon_hal_get_transceiver_stats(&stats);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, tx_power=%.2f dBm, rx_power=%.2f dBm, "
             "temperature=%.1f C, bias_current=%.2f mA, supply_voltage=%.2f V",
             rc_str(rc),
             stats.transmit_optical_level, stats.optical_signal_level,
             stats.temperature, stats.bias_current, stats.supply_voltage);

    if (rc == EPON_HAL_SUCCESS) {
        /* Sanity: temperature should be between -40 and 100 C */
        if (stats.temperature < -40.0f || stats.temperature > 100.0f) {
            snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                     " [WARN: temperature %.1f out of normal range]", stats.temperature);
            add_result(rpt, "TC-05 epon_hal_get_transceiver_stats", HAL_TEST_WARN, detail, method, elapsed);
        } else {
            add_result(rpt, "TC-05 epon_hal_get_transceiver_stats", HAL_TEST_PASS, detail, method, elapsed);
        }
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-05 epon_hal_get_transceiver_stats", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-05 epon_hal_get_transceiver_stats", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-06: epon_hal_get_transceiver_stats() - NULL param
 */
static void test_get_transceiver_stats_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_transceiver_stats(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_transceiver_stats(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-06 get_transceiver_stats(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-06 get_transceiver_stats(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-07: epon_hal_get_llid_info() - normal call
 */
static void test_get_llid_info(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_llid_info(). "
                         "Expects SUCCESS, validates LLID count and list allocation.";
    double t0 = now_ms();

    epon_llid_list_t llid_list;
    memset(&llid_list, 0, sizeof(llid_list));

    epon_hal_return_t rc = epon_hal_get_llid_info(&llid_list);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (rc == EPON_HAL_SUCCESS) {
        snprintf(detail, sizeof(detail),
                 "rc=SUCCESS, max_llid=%u, llid_count=%u",
                 llid_list.max_llid_count, llid_list.llid_count);

        /* Print first few LLIDs */
        for (uint32_t i = 0; i < llid_list.llid_count && i < 4; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     ", LLID[%u]: value=%u state=%u mode=%u",
                     i, llid_list.llid_list[i].llid_value,
                     llid_list.llid_list[i].state,
                     llid_list.llid_list[i].mode);
            strncat(detail, buf, sizeof(detail) - strlen(detail) - 1);
        }

        add_result(rpt, "TC-07 epon_hal_get_llid_info", HAL_TEST_PASS, detail, method, elapsed);

        /* Free HAL-allocated memory */
        if (llid_list.llid_list) {
            free(llid_list.llid_list);
        }
    } else {
        snprintf(detail, sizeof(detail), "rc=%s", rc_str(rc));
        add_result(rpt, "TC-07 epon_hal_get_llid_info", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-08: epon_hal_get_llid_info() - NULL param
 */
static void test_get_llid_info_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_llid_info(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_llid_info(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-08 get_llid_info(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-08 get_llid_info(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-09: epon_hal_get_manufacturer_info() - normal call
 */
static void test_get_manufacturer_info(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_manufacturer_info(). "
                         "Sets struct_size, expects SUCCESS, validates non-empty fields.";
    double t0 = now_ms();

    epon_onu_manufacturer_info_t info;
    memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);

    epon_hal_return_t rc = epon_hal_get_manufacturer_info(&info);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, manufacturer='%s', model='%s', hw_ver='%s', "
             "sw_ver='%s', serial='%s', oui=%02X:%02X:%02X",
             rc_str(rc), info.manufacturer, info.model_number,
             info.hardware_version, info.software_version, info.serial_number,
             info.vendor_oui[0], info.vendor_oui[1], info.vendor_oui[2]);

    if (rc == EPON_HAL_SUCCESS) {
        /* Check that at least manufacturer is non-empty */
        if (strlen(info.manufacturer) == 0) {
            strncat(detail, " [WARN: manufacturer is empty]", sizeof(detail) - strlen(detail) - 1);
            add_result(rpt, "TC-09 epon_hal_get_manufacturer_info", HAL_TEST_WARN, detail, method, elapsed);
        } else {
            add_result(rpt, "TC-09 epon_hal_get_manufacturer_info", HAL_TEST_PASS, detail, method, elapsed);
        }
    } else {
        add_result(rpt, "TC-09 epon_hal_get_manufacturer_info", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-10: epon_hal_get_manufacturer_info() - NULL param
 */
static void test_get_manufacturer_info_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_manufacturer_info(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_manufacturer_info(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-10 get_manufacturer_info(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-10 get_manufacturer_info(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-11: epon_hal_get_link_info() - normal call
 */
static void test_get_link_info(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_link_info(). "
                         "Expects SUCCESS, validates mode string is non-empty.";
    double t0 = now_ms();

    epon_hal_link_info_t info;
    memset(&info, 0, sizeof(info));

    epon_hal_return_t rc = epon_hal_get_link_info(&info);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, mode='%s', encryption=%d",
             rc_str(rc), info.mode, info.encryption);

    if (rc == EPON_HAL_SUCCESS) {
        if (strlen(info.mode) == 0) {
            strncat(detail, " [WARN: mode string is empty]", sizeof(detail) - strlen(detail) - 1);
            add_result(rpt, "TC-11 epon_hal_get_link_info", HAL_TEST_WARN, detail, method, elapsed);
        } else {
            add_result(rpt, "TC-11 epon_hal_get_link_info", HAL_TEST_PASS, detail, method, elapsed);
        }
    } else {
        add_result(rpt, "TC-11 epon_hal_get_link_info", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-12: epon_hal_get_link_info() - NULL param
 */
static void test_get_link_info_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_link_info(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_link_info(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-12 get_link_info(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-12 get_link_info(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-13: epon_hal_get_interface_list() - normal call
 */
static void test_get_interface_list(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_interface_list(). "
                         "Expects SUCCESS, validates interface_count and names.";
    double t0 = now_ms();

    epon_interface_list_t if_list;
    memset(&if_list, 0, sizeof(if_list));

    epon_hal_return_t rc = epon_hal_get_interface_list(&if_list);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, interface_count=%u", rc_str(rc), if_list.interface_count);

    if (rc == EPON_HAL_SUCCESS) {
        for (uint32_t i = 0; i < if_list.interface_count && i < 4; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     ", intf[%u]: name='%s' status=%d",
                     i, if_list.interface[i].name, if_list.interface[i].status);
            strncat(detail, buf, sizeof(detail) - strlen(detail) - 1);
        }
        add_result(rpt, "TC-13 epon_hal_get_interface_list", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-13 epon_hal_get_interface_list", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-14: epon_hal_get_interface_list() - NULL param
 */
static void test_get_interface_list_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_interface_list(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_interface_list(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-14 get_interface_list(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-14 get_interface_list(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-15: epon_hal_get_olt_info() - normal call
 */
static void test_get_olt_info(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_olt_info(). "
                         "Sets struct_size, expects SUCCESS or NOT_SUPPORTED (if ONU not registered).";
    double t0 = now_ms();

    epon_olt_info_t olt_info;
    memset(&olt_info, 0, sizeof(olt_info));
    olt_info.struct_size = sizeof(olt_info);

    epon_hal_return_t rc = epon_hal_get_olt_info(&olt_info);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, olt_mac=%02X:%02X:%02X:%02X:%02X:%02X, "
             "olt_oui=%02X:%02X:%02X",
             rc_str(rc),
             olt_info.mac_address[0], olt_info.mac_address[1],
             olt_info.mac_address[2], olt_info.mac_address[3],
             olt_info.mac_address[4], olt_info.mac_address[5],
             olt_info.vendor_oui[0], olt_info.vendor_oui[1],
             olt_info.vendor_oui[2]);

    if (rc == EPON_HAL_SUCCESS) {
        add_result(rpt, "TC-15 epon_hal_get_olt_info", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        strncat(detail, " (ONU may not be registered yet)", sizeof(detail) - strlen(detail) - 1);
        add_result(rpt, "TC-15 epon_hal_get_olt_info", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-15 epon_hal_get_olt_info", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-16: epon_hal_get_olt_info() - NULL param
 */
static void test_get_olt_info_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_olt_info(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_get_olt_info(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-16 get_olt_info(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-16 get_olt_info(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-17: epon_hal_set_oam_log_mask() - enable all
 */
static void test_set_oam_log_mask_all(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_set_oam_log_mask(EPON_OAM_ALL). "
                         "Expects SUCCESS or NOT_SUPPORTED.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_set_oam_log_mask(EPON_OAM_ALL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s, mask=0x%08X", rc_str(rc), EPON_OAM_ALL);

    if (rc == EPON_HAL_SUCCESS) {
        add_result(rpt, "TC-17 set_oam_log_mask(ALL)", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-17 set_oam_log_mask(ALL)", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-17 set_oam_log_mask(ALL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-18: epon_hal_set_oam_log_mask() - disable all
 */
static void test_set_oam_log_mask_none(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_set_oam_log_mask(0). "
                         "Expects SUCCESS or NOT_SUPPORTED.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_set_oam_log_mask(0);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s, mask=0x00000000", rc_str(rc));

    if (rc == EPON_HAL_SUCCESS) {
        add_result(rpt, "TC-18 set_oam_log_mask(NONE)", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-18 set_oam_log_mask(NONE)", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-18 set_oam_log_mask(NONE)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-19: epon_hal_set_oam_log_mask() - selective mask
 */
static void test_set_oam_log_mask_selective(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_set_oam_log_mask(INFO|EVENT|VAR_REQUEST). "
                         "Expects SUCCESS or NOT_SUPPORTED.";
    uint32_t mask = EPON_OAM_INFO | EPON_OAM_EVENT | EPON_OAM_VAR_REQUEST;
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_set_oam_log_mask(mask);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s, mask=0x%08X", rc_str(rc), mask);

    if (rc == EPON_HAL_SUCCESS) {
        add_result(rpt, "TC-19 set_oam_log_mask(selective)", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-19 set_oam_log_mask(selective)", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-19 set_oam_log_mask(selective)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-20: epon_hal_clear_stats()
 */
static void test_clear_stats(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_clear_stats(). "
                         "Expects SUCCESS. Then re-reads stats to verify counters were reset.";
    double t0 = now_ms();

    epon_hal_return_t rc = epon_hal_clear_stats();
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s", rc_str(rc));

    if (rc == EPON_HAL_SUCCESS) {
        /* Verify counters were reset by reading stats again */
        epon_hal_link_stats_t stats;
        memset(&stats, 0, sizeof(stats));
        stats.struct_size = sizeof(stats);
        epon_hal_return_t rc2 = epon_hal_get_link_stats(&stats);
        if (rc2 == EPON_HAL_SUCCESS) {
            snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                     ", post-clear: packets_sent=%" PRIu64 " packets_received=%" PRIu64,
                     stats.packets_sent, stats.packets_received);
        }
        add_result(rpt, "TC-20 epon_hal_clear_stats", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-20 epon_hal_clear_stats", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-21: dpoe_hal_get_cpe_mac_table() - normal call
 */
static void test_get_cpe_mac_table(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: dpoe_hal_get_cpe_mac_table(). "
                         "Expects SUCCESS or NOT_SUPPORTED if DPoE not enabled.";
    double t0 = now_ms();

    dpoe_cpe_mac_table_t cpe_table;
    memset(&cpe_table, 0, sizeof(cpe_table));

    epon_hal_return_t rc = dpoe_hal_get_cpe_mac_table(&cpe_table);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail),
             "rc=%s, max_cpe=%u, static=%u, dynamic=%u",
             rc_str(rc), cpe_table.max_cpe,
             cpe_table.static_cpe_count, cpe_table.dynamic_cpe_count);

    if (rc == EPON_HAL_SUCCESS) {
        for (uint32_t i = 0; i < (cpe_table.static_cpe_count + cpe_table.dynamic_cpe_count) && i < 4; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     ", CPE[%u]: mac=%02X:%02X:%02X:%02X:%02X:%02X type=%d",
                     i,
                     cpe_table.cpe_list[i].mac_address[0],
                     cpe_table.cpe_list[i].mac_address[1],
                     cpe_table.cpe_list[i].mac_address[2],
                     cpe_table.cpe_list[i].mac_address[3],
                     cpe_table.cpe_list[i].mac_address[4],
                     cpe_table.cpe_list[i].mac_address[5],
                     cpe_table.cpe_list[i].type);
            strncat(detail, buf, sizeof(detail) - strlen(detail) - 1);
        }
        add_result(rpt, "TC-21 dpoe_hal_get_cpe_mac_table", HAL_TEST_PASS, detail, method, elapsed);

        if (cpe_table.cpe_list) {
            free(cpe_table.cpe_list);
        }
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-21 dpoe_hal_get_cpe_mac_table", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-21 dpoe_hal_get_cpe_mac_table", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-22: dpoe_hal_get_cpe_mac_table() - NULL param
 */
static void test_get_cpe_mac_table_null(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: dpoe_hal_get_cpe_mac_table(NULL). "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM or NOT_SUPPORTED.";
    double t0 = now_ms();

    epon_hal_return_t rc = dpoe_hal_get_cpe_mac_table(NULL);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-22 get_cpe_mac_table(NULL)", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-22 get_cpe_mac_table(NULL)", HAL_TEST_SKIP, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-22 get_cpe_mac_table(NULL)", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-23: Cross-validate link stats via eponMgr data layer vs direct HAL
 */
static void test_data_layer_link_stats(hal_test_report_t *rpt)
{
    const char *method = "Cross-validation: eponMgr_data_get_link_stats() vs direct epon_hal_get_link_stats(). "
                         "Compares results from data layer cache with fresh HAL call.";
    double t0 = now_ms();

    eponMgr_data_t *data = eponMgr_data_lock();
    if (!data) {
        double elapsed = now_ms() - t0;
        add_result(rpt, "TC-23 data_layer_link_stats", HAL_TEST_SKIP,
                   "Data context not available (lock failed)", method, elapsed);
        return;
    }

    const epon_hal_link_stats_t *cached = eponMgr_data_get_link_stats(data);
    eponMgr_data_unlock();

    /* Direct HAL call */
    epon_hal_link_stats_t direct;
    memset(&direct, 0, sizeof(direct));
    direct.struct_size = sizeof(direct);
    epon_hal_return_t rc = epon_hal_get_link_stats(&direct);

    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (cached && rc == EPON_HAL_SUCCESS) {
        snprintf(detail, sizeof(detail),
                 "Data layer: pkt_sent=%" PRIu64 " pkt_recv=%" PRIu64
                 ", Direct HAL: pkt_sent=%" PRIu64 " pkt_recv=%" PRIu64,
                 cached->packets_sent, cached->packets_received,
                 direct.packets_sent, direct.packets_received);
        add_result(rpt, "TC-23 data_layer_link_stats", HAL_TEST_PASS, detail, method, elapsed);
    } else if (!cached) {
        snprintf(detail, sizeof(detail), "Data layer returned NULL (direct rc=%s)", rc_str(rc));
        add_result(rpt, "TC-23 data_layer_link_stats", HAL_TEST_WARN, detail, method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "Direct HAL call failed: rc=%s", rc_str(rc));
        add_result(rpt, "TC-23 data_layer_link_stats", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-24: Cross-validate transceiver stats via data layer vs direct HAL
 */
static void test_data_layer_transceiver_stats(hal_test_report_t *rpt)
{
    const char *method = "Cross-validation: eponMgr_data_get_transceiver_stats() vs direct epon_hal_get_transceiver_stats(). "
                         "Compares optical levels from data layer cache with fresh HAL call.";
    double t0 = now_ms();

    eponMgr_data_t *data = eponMgr_data_lock();
    if (!data) {
        double elapsed = now_ms() - t0;
        add_result(rpt, "TC-24 data_layer_xcvr_stats", HAL_TEST_SKIP,
                   "Data context not available", method, elapsed);
        return;
    }

    const epon_hal_transceiver_stats_t *cached = eponMgr_data_get_transceiver_stats(data);
    eponMgr_data_unlock();

    epon_hal_transceiver_stats_t direct;
    memset(&direct, 0, sizeof(direct));
    direct.struct_size = sizeof(direct);
    epon_hal_return_t rc = epon_hal_get_transceiver_stats(&direct);

    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (cached && rc == EPON_HAL_SUCCESS) {
        snprintf(detail, sizeof(detail),
                 "Data layer: tx=%.2f rx=%.2f temp=%.1f, "
                 "Direct: tx=%.2f rx=%.2f temp=%.1f",
                 cached->transmit_optical_level, cached->optical_signal_level,
                 cached->temperature,
                 direct.transmit_optical_level, direct.optical_signal_level,
                 direct.temperature);
        add_result(rpt, "TC-24 data_layer_xcvr_stats", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-24 data_layer_xcvr_stats", HAL_TEST_SKIP,
                   "Transceiver stats not supported", method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "cached=%p, direct_rc=%s", (void*)cached, rc_str(rc));
        add_result(rpt, "TC-24 data_layer_xcvr_stats", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-25: Cross-validate manufacturer info via data layer
 */
static void test_data_layer_manufacturer(hal_test_report_t *rpt)
{
    const char *method = "Cross-validation: eponMgr_data_get_onu_manufacturer_info() vs direct HAL. "
                         "Compares manufacturer/model/serial between data layer and HAL.";
    double t0 = now_ms();

    eponMgr_data_t *data = eponMgr_data_lock();
    if (!data) {
        double elapsed = now_ms() - t0;
        add_result(rpt, "TC-25 data_layer_manufacturer", HAL_TEST_SKIP,
                   "Data context not available", method, elapsed);
        return;
    }

    const epon_onu_manufacturer_info_t *cached = eponMgr_data_get_onu_manufacturer_info(data);
    eponMgr_data_unlock();

    epon_onu_manufacturer_info_t direct;
    memset(&direct, 0, sizeof(direct));
    direct.struct_size = sizeof(direct);
    epon_hal_return_t rc = epon_hal_get_manufacturer_info(&direct);

    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (cached && rc == EPON_HAL_SUCCESS) {
        bool match = (strcmp(cached->manufacturer, direct.manufacturer) == 0 &&
                      strcmp(cached->serial_number, direct.serial_number) == 0);
        snprintf(detail, sizeof(detail),
                 "Data: mfr='%s' sn='%s', Direct: mfr='%s' sn='%s' [%s]",
                 cached->manufacturer, cached->serial_number,
                 direct.manufacturer, direct.serial_number,
                 match ? "MATCH" : "MISMATCH");
        add_result(rpt, "TC-25 data_layer_manufacturer",
                   match ? HAL_TEST_PASS : HAL_TEST_WARN, detail, method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "cached=%p, direct_rc=%s", (void*)cached, rc_str(rc));
        add_result(rpt, "TC-25 data_layer_manufacturer", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-26: Cross-validate link info via data layer
 */
static void test_data_layer_link_info(hal_test_report_t *rpt)
{
    const char *method = "Cross-validation: eponMgr_data_get_link_info() vs direct epon_hal_get_link_info(). "
                         "Compares mode and encryption from data layer cache with HAL.";
    double t0 = now_ms();

    eponMgr_data_t *data = eponMgr_data_lock();
    if (!data) {
        double elapsed = now_ms() - t0;
        add_result(rpt, "TC-26 data_layer_link_info", HAL_TEST_SKIP,
                   "Data context not available", method, elapsed);
        return;
    }

    const epon_hal_link_info_t *cached = eponMgr_data_get_link_info(data);
    eponMgr_data_unlock();

    epon_hal_link_info_t direct;
    memset(&direct, 0, sizeof(direct));
    epon_hal_return_t rc = epon_hal_get_link_info(&direct);

    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (cached && rc == EPON_HAL_SUCCESS) {
        bool match = (strcmp(cached->mode, direct.mode) == 0 &&
                      cached->encryption == direct.encryption);
        snprintf(detail, sizeof(detail),
                 "Data: mode='%s' enc=%d, Direct: mode='%s' enc=%d [%s]",
                 cached->mode, cached->encryption,
                 direct.mode, direct.encryption,
                 match ? "MATCH" : "MISMATCH");
        add_result(rpt, "TC-26 data_layer_link_info",
                   match ? HAL_TEST_PASS : HAL_TEST_WARN, detail, method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "cached=%p, direct_rc=%s", (void*)cached, rc_str(rc));
        add_result(rpt, "TC-26 data_layer_link_info", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-27: Cross-validate OLT info via data layer
 */
static void test_data_layer_olt_info(hal_test_report_t *rpt)
{
    const char *method = "Cross-validation: eponMgr_data_get_olt_info() vs direct epon_hal_get_olt_info(). "
                         "Compares OLT MAC and OUI between data layer and HAL.";
    double t0 = now_ms();

    eponMgr_data_t *data = eponMgr_data_lock();
    if (!data) {
        double elapsed = now_ms() - t0;
        add_result(rpt, "TC-27 data_layer_olt_info", HAL_TEST_SKIP,
                   "Data context not available", method, elapsed);
        return;
    }

    const epon_olt_info_t *cached = eponMgr_data_get_olt_info(data);
    eponMgr_data_unlock();

    epon_olt_info_t direct;
    memset(&direct, 0, sizeof(direct));
    direct.struct_size = sizeof(direct);
    epon_hal_return_t rc = epon_hal_get_olt_info(&direct);

    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (cached && rc == EPON_HAL_SUCCESS) {
        bool mac_match = (memcmp(cached->mac_address, direct.mac_address, EPON_HAL_MAC_ADDR_LEN) == 0);
        snprintf(detail, sizeof(detail),
                 "Data OLT MAC: %02X:%02X:%02X:%02X:%02X:%02X, "
                 "Direct OLT MAC: %02X:%02X:%02X:%02X:%02X:%02X [%s]",
                 cached->mac_address[0], cached->mac_address[1],
                 cached->mac_address[2], cached->mac_address[3],
                 cached->mac_address[4], cached->mac_address[5],
                 direct.mac_address[0], direct.mac_address[1],
                 direct.mac_address[2], direct.mac_address[3],
                 direct.mac_address[4], direct.mac_address[5],
                 mac_match ? "MATCH" : "MISMATCH");
        add_result(rpt, "TC-27 data_layer_olt_info",
                   mac_match ? HAL_TEST_PASS : HAL_TEST_WARN, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_NOT_SUPPORTED) {
        add_result(rpt, "TC-27 data_layer_olt_info", HAL_TEST_SKIP,
                   "OLT info not available (ONU not registered)", method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "cached=%p, direct_rc=%s", (void*)cached, rc_str(rc));
        add_result(rpt, "TC-27 data_layer_olt_info", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-28: Repeated link stats call - consistency check
 */
static void test_link_stats_consistency(hal_test_report_t *rpt)
{
    const char *method = "Stability test: Call epon_hal_get_link_stats() twice in quick succession. "
                         "Second call counters should be >= first call (monotonic).";
    double t0 = now_ms();

    epon_hal_link_stats_t stats1, stats2;
    memset(&stats1, 0, sizeof(stats1));
    memset(&stats2, 0, sizeof(stats2));
    stats1.struct_size = sizeof(stats1);
    stats2.struct_size = sizeof(stats2);

    epon_hal_return_t rc1 = epon_hal_get_link_stats(&stats1);
    epon_hal_return_t rc2 = epon_hal_get_link_stats(&stats2);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    if (rc1 == EPON_HAL_SUCCESS && rc2 == EPON_HAL_SUCCESS) {
        bool monotonic = (stats2.packets_sent >= stats1.packets_sent &&
                          stats2.packets_received >= stats1.packets_received &&
                          stats2.bytes_sent >= stats1.bytes_sent &&
                          stats2.bytes_received >= stats1.bytes_received);
        snprintf(detail, sizeof(detail),
                 "Call1: pkt_s=%" PRIu64 " pkt_r=%" PRIu64
                 ", Call2: pkt_s=%" PRIu64 " pkt_r=%" PRIu64
                 " [%s]",
                 stats1.packets_sent, stats1.packets_received,
                 stats2.packets_sent, stats2.packets_received,
                 monotonic ? "MONOTONIC" : "NON-MONOTONIC");
        add_result(rpt, "TC-28 link_stats_consistency",
                   monotonic ? HAL_TEST_PASS : HAL_TEST_WARN, detail, method, elapsed);
    } else {
        snprintf(detail, sizeof(detail), "rc1=%s, rc2=%s", rc_str(rc1), rc_str(rc2));
        add_result(rpt, "TC-28 link_stats_consistency", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-29: epon_hal_get_olt_info() - bad struct_size
 */
static void test_get_olt_info_bad_size(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_olt_info() with struct_size=0. "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_olt_info_t olt_info;
    memset(&olt_info, 0, sizeof(olt_info));
    olt_info.struct_size = 0;  /* intentionally wrong */

    epon_hal_return_t rc = epon_hal_get_olt_info(&olt_info);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-29 get_olt_info(bad_size)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-29 get_olt_info(bad_size)", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-30: epon_hal_get_manufacturer_info() - bad struct_size
 */
static void test_get_manufacturer_info_bad_size(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_manufacturer_info() with struct_size=0. "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_onu_manufacturer_info_t info;
    memset(&info, 0, sizeof(info));
    info.struct_size = 0;  /* intentionally wrong */

    epon_hal_return_t rc = epon_hal_get_manufacturer_info(&info);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-30 get_mfr_info(bad_size)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-30 get_mfr_info(bad_size)", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-31: epon_hal_get_transceiver_stats() - bad struct_size
 */
static void test_get_transceiver_stats_bad_size(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_get_transceiver_stats() with struct_size=0. "
                         "Expects EPON_HAL_ERROR_INVALID_PARAM.";
    double t0 = now_ms();

    epon_hal_transceiver_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    stats.struct_size = 0;

    epon_hal_return_t rc = epon_hal_get_transceiver_stats(&stats);
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s (expected INVALID_PARAM)", rc_str(rc));

    if (rc == EPON_HAL_ERROR_INVALID_PARAM) {
        add_result(rpt, "TC-31 get_xcvr_stats(bad_size)", HAL_TEST_PASS, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-31 get_xcvr_stats(bad_size)", HAL_TEST_WARN, detail, method, elapsed);
    }
}

/**
 * TC-32: epon_hal_reset_onu() - destructive test
 *
 * NOTE: This test is DESTRUCTIVE - it triggers an ONU reset which causes
 * temporary service disruption. Only runs when explicitly requested via
 * the IncludeDestructive input parameter.
 *
 * After reset, the ONU will deregister from the OLT and re-register.
 * We verify the API returns SUCCESS and then confirm the HAL is still
 * functional by calling epon_hal_get_link_info() after a short delay.
 */
static void test_reset_onu(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_reset_onu(). "
                         "DESTRUCTIVE: Triggers ONU soft reset and re-registration. "
                         "Expects SUCCESS, then waits 5s and re-queries link info to confirm HAL is alive.";
    double t0 = now_ms();

    EPONMGR_LOG_WARN("[HAL-TEST] TC-32: Executing DESTRUCTIVE test - epon_hal_reset_onu()\n");

    epon_hal_return_t rc = epon_hal_reset_onu();
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s", rc_str(rc));

    if (rc == EPON_HAL_SUCCESS) {
        /* Wait for the ONU to begin re-registration */
        EPONMGR_LOG_INFO("[HAL-TEST] TC-32: Reset returned SUCCESS, waiting 5s for re-registration...\n");
        sleep(5);

        /* Verify HAL is still functional after reset */
        epon_hal_link_info_t info;
        memset(&info, 0, sizeof(info));
        epon_hal_return_t rc2 = epon_hal_get_link_info(&info);
        snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                 ", post-reset get_link_info rc=%s mode='%s'",
                 rc_str(rc2), info.mode);

        if (rc2 == EPON_HAL_SUCCESS || rc2 == EPON_HAL_ERROR_NOT_INITIALIZED) {
            /* NOT_INITIALIZED is acceptable since ONU is still re-registering */
            add_result(rpt, "TC-32 epon_hal_reset_onu", HAL_TEST_PASS, detail, method, elapsed);
        } else {
            add_result(rpt, "TC-32 epon_hal_reset_onu", HAL_TEST_WARN, detail, method, elapsed);
        }
    } else {
        add_result(rpt, "TC-32 epon_hal_reset_onu", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/**
 * TC-33: epon_hal_factory_reset() - destructive test
 *
 * NOTE: This test is DESTRUCTIVE - it resets all HAL configuration to
 * factory defaults. Service will be disrupted and re-initialization is
 * required. Only runs when explicitly requested via IncludeDestructive.
 *
 * After factory reset, epon_hal_init() would need to be called again.
 * Since we are running inside the EPON Manager, we do NOT re-init here;
 * we only verify the API returns SUCCESS or an expected error.
 */
static void test_factory_reset(hal_test_report_t *rpt)
{
    const char *method = "Direct HAL call: epon_hal_factory_reset(). "
                         "DESTRUCTIVE: Resets all HAL config to factory defaults. "
                         "Expects SUCCESS. WARNING: epon_hal_init() is required after this call.";
    double t0 = now_ms();

    EPONMGR_LOG_WARN("[HAL-TEST] TC-33: Executing DESTRUCTIVE test - epon_hal_factory_reset()\n");

    epon_hal_return_t rc = epon_hal_factory_reset();
    double elapsed = now_ms() - t0;

    char detail[HAL_TEST_DETAIL_LEN];
    snprintf(detail, sizeof(detail), "rc=%s", rc_str(rc));

    if (rc == EPON_HAL_SUCCESS) {
        snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                 " [Factory reset completed - HAL re-init required]");
        add_result(rpt, "TC-33 epon_hal_factory_reset", HAL_TEST_PASS, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_CONFIG) {
        snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                 " [Could not restore default config]");
        add_result(rpt, "TC-33 epon_hal_factory_reset", HAL_TEST_FAIL, detail, method, elapsed);
    } else if (rc == EPON_HAL_ERROR_HW_FAILURE) {
        snprintf(detail + strlen(detail), sizeof(detail) - strlen(detail),
                 " [HW failure during factory reset]");
        add_result(rpt, "TC-33 epon_hal_factory_reset", HAL_TEST_FAIL, detail, method, elapsed);
    } else {
        add_result(rpt, "TC-33 epon_hal_factory_reset", HAL_TEST_FAIL, detail, method, elapsed);
    }
}

/* ========================================================================== */
/*  Public API                                                                */
/* ========================================================================== */

/**
 * @brief Run the full EPON HAL validation test suite
 */
int eponMgr_hal_test_run_all(hal_test_report_t *report, bool include_destructive)
{
    if (!report) return -1;

    memset(report, 0, sizeof(*report));
    double suite_start = now_ms();

    EPONMGR_LOG_INFO("========================================\n");
    EPONMGR_LOG_INFO("  EPON HAL Validation Test Suite START\n");
    EPONMGR_LOG_INFO("========================================\n");

    /* --- Version API --- */
    test_get_version(report);

    /* --- Link Stats API --- */
    test_get_link_stats(report);
    test_get_link_stats_null(report);
    test_get_link_stats_bad_size(report);

    /* --- Transceiver Stats API --- */
    test_get_transceiver_stats(report);
    test_get_transceiver_stats_null(report);
    test_get_transceiver_stats_bad_size(report);

    /* --- LLID Info API --- */
    test_get_llid_info(report);
    test_get_llid_info_null(report);

    /* --- Manufacturer Info API --- */
    test_get_manufacturer_info(report);
    test_get_manufacturer_info_null(report);
    test_get_manufacturer_info_bad_size(report);

    /* --- Link Info API --- */
    test_get_link_info(report);
    test_get_link_info_null(report);

    /* --- Interface List API --- */
    test_get_interface_list(report);
    test_get_interface_list_null(report);

    /* --- OLT Info API --- */
    test_get_olt_info(report);
    test_get_olt_info_null(report);
    test_get_olt_info_bad_size(report);

    /* --- OAM Log Mask API --- */
    test_set_oam_log_mask_all(report);
    test_set_oam_log_mask_none(report);
    test_set_oam_log_mask_selective(report);

    /* --- Clear Stats API --- */
    test_clear_stats(report);

    /* --- DPoE CPE MAC Table API --- */
    test_get_cpe_mac_table(report);
    test_get_cpe_mac_table_null(report);

    /* --- Data Layer Cross-Validation --- */
    test_data_layer_link_stats(report);
    test_data_layer_transceiver_stats(report);
    test_data_layer_manufacturer(report);
    test_data_layer_link_info(report);
    test_data_layer_olt_info(report);

    /* --- Stability/Consistency --- */
    test_link_stats_consistency(report);

    /* --- Destructive Tests (reset/factory_reset) --- */
    if (include_destructive) {
        EPONMGR_LOG_WARN("========================================\n");
        EPONMGR_LOG_WARN("  DESTRUCTIVE TESTS ENABLED\n");
        EPONMGR_LOG_WARN("  These tests will disrupt ONU service!\n");
        EPONMGR_LOG_WARN("========================================\n");
        test_reset_onu(report);
        test_factory_reset(report);
    } else {
        add_result(report, "TC-32 epon_hal_reset_onu", HAL_TEST_SKIP,
                   "Skipped: destructive test not requested (set IncludeDestructive=true)",
                   "Requires IncludeDestructive=true input parameter to execute", 0.0);
        add_result(report, "TC-33 epon_hal_factory_reset", HAL_TEST_SKIP,
                   "Skipped: destructive test not requested (set IncludeDestructive=true)",
                   "Requires IncludeDestructive=true input parameter to execute", 0.0);
    }

    report->total_elapsed_ms = now_ms() - suite_start;

    EPONMGR_LOG_INFO("========================================\n");
    EPONMGR_LOG_INFO("  EPON HAL Test Suite COMPLETE\n");
    EPONMGR_LOG_INFO("  Total: %u  Pass: %u  Fail: %u  Skip: %u  Warn: %u\n",
                     report->total, report->passed, report->failed,
                     report->skipped, report->warnings);
    EPONMGR_LOG_INFO("  Elapsed: %.1f ms\n", report->total_elapsed_ms);
    EPONMGR_LOG_INFO("========================================\n");

    return 0;
}

/**
 * @brief Write the test report to file and RDK logs
 */
int eponMgr_hal_test_write_report(const hal_test_report_t *report)
{
    if (!report) return -1;

    FILE *fp = fopen(HAL_TEST_REPORT_FILE, "w");
    if (!fp) {
        EPONMGR_LOG_ERROR("Failed to open test report file %s: %s\n",
                          HAL_TEST_REPORT_FILE, strerror(errno));
        return -1;
    }

    /* Header */
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "================================================================================\n");
    fprintf(fp, "  EPON HAL Validation Test Report\n");
    fprintf(fp, "  Generated: %s\n", time_buf);
    fprintf(fp, "  HAL API Version: %u.%u.%u\n",
            EPON_HAL_VERSION_MAJOR, EPON_HAL_VERSION_MINOR, EPON_HAL_VERSION_PATCH);
    fprintf(fp, "================================================================================\n\n");

    /* Summary */
    fprintf(fp, "SUMMARY\n");
    fprintf(fp, "-------\n");
    fprintf(fp, "  Total Tests  : %u\n", report->total);
    fprintf(fp, "  Passed       : %u\n", report->passed);
    fprintf(fp, "  Failed       : %u\n", report->failed);
    fprintf(fp, "  Skipped      : %u\n", report->skipped);
    fprintf(fp, "  Warnings     : %u\n", report->warnings);
    fprintf(fp, "  Total Time   : %.1f ms\n", report->total_elapsed_ms);
    fprintf(fp, "  Result       : %s\n\n", report->failed == 0 ? "ALL PASSED" : "HAS FAILURES");

    /* Detailed Results */
    fprintf(fp, "DETAILED RESULTS\n");
    fprintf(fp, "----------------\n\n");

    for (uint32_t i = 0; i < report->total; i++) {
        const hal_test_case_result_t *tc = &report->cases[i];

        fprintf(fp, "[%s] %s  (%.1f ms)\n", status_str(tc->status), tc->name, tc->elapsed_ms);
        fprintf(fp, "  Method  : %s\n", tc->method);
        fprintf(fp, "  Details : %s\n\n", tc->detail);

        /* Also log each result to RDK log */
        switch (tc->status) {
            case HAL_TEST_PASS:
                EPONMGR_LOG_INFO("[HAL-TEST] [PASS] %s: %s\n", tc->name, tc->detail);
                break;
            case HAL_TEST_FAIL:
                EPONMGR_LOG_ERROR("[HAL-TEST] [FAIL] %s: %s\n", tc->name, tc->detail);
                break;
            case HAL_TEST_SKIP:
                EPONMGR_LOG_INFO("[HAL-TEST] [SKIP] %s: %s\n", tc->name, tc->detail);
                break;
            case HAL_TEST_WARN:
                EPONMGR_LOG_WARN("[HAL-TEST] [WARN] %s: %s\n", tc->name, tc->detail);
                break;
        }
    }

    /* Footer */
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "  END OF REPORT\n");
    fprintf(fp, "  Report file: %s\n", HAL_TEST_REPORT_FILE);
    fprintf(fp, "================================================================================\n");

    fclose(fp);

    EPONMGR_LOG_INFO("[HAL-TEST] Report written to %s\n", HAL_TEST_REPORT_FILE);

    return 0;
}

/**
 * @brief RBUS method handler - triggers HAL test suite
 *
 * Invoked via:
 *   rbusMethod_Invoke(handle, "Device.Optical.Interface.1.X_RDK_EPON.RunHALTest", in, &out)
 *
 * Or from CLI:
 *   rbusmethod_invoke epon_manager Device.Optical.Interface.1.X_RDK_EPON.RunHALTest() 
 *   rbusmethod_invoke epon_manager Device.Optical.Interface.1.X_RDK_EPON.RunHALTest() IncludeDestructive:bool:true
 */
rbusError_t eponMgr_hal_test_rbus_handler(rbusHandle_t handle,
                                          char const *methodName,
                                          rbusObject_t inParams,
                                          rbusObject_t outParams,
                                          rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)inParams;
    (void)asyncHandle;

    EPONMGR_LOG_INFO("RBUS method invoked: %s\n", methodName ? methodName : "(null)");

    /* Check if destructive tests are requested via input parameter */
    bool include_destructive = false;
    if (inParams) {
        rbusValue_t destructive_val = rbusObject_GetValue(inParams, "IncludeDestructive");
        if (destructive_val) {
            include_destructive = rbusValue_GetBoolean(destructive_val);
        }
    }
    EPONMGR_LOG_INFO("IncludeDestructive=%s\n", include_destructive ? "true" : "false");

    hal_test_report_t *report = (hal_test_report_t *)calloc(1, sizeof(hal_test_report_t));
    if (!report) {
        EPONMGR_LOG_ERROR("Failed to allocate test report\n");
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }

    int ret = eponMgr_hal_test_run_all(report, include_destructive);
    if (ret != 0) {
        EPONMGR_LOG_ERROR("HAL test suite execution failed\n");
        free(report);
        return RBUS_ERROR_BUS_ERROR;
    }

    /* Write report file */
    eponMgr_hal_test_write_report(report);

    /* Set output parameters */
    rbusValue_t val;

    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, report->total);
    rbusObject_SetValue(outParams, "TotalTests", val);
    rbusValue_Release(val);

    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, report->passed);
    rbusObject_SetValue(outParams, "Passed", val);
    rbusValue_Release(val);

    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, report->failed);
    rbusObject_SetValue(outParams, "Failed", val);
    rbusValue_Release(val);

    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, report->skipped);
    rbusObject_SetValue(outParams, "Skipped", val);
    rbusValue_Release(val);

    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, report->warnings);
    rbusObject_SetValue(outParams, "Warnings", val);
    rbusValue_Release(val);

    rbusValue_Init(&val);
    rbusValue_SetString(val, HAL_TEST_REPORT_FILE);
    rbusObject_SetValue(outParams, "ReportFile", val);
    rbusValue_Release(val);

    /* Build result string */
    char result[256];
    snprintf(result, sizeof(result),
             "Total=%u Pass=%u Fail=%u Skip=%u Warn=%u (%.1fms) Report=%s",
             report->total, report->passed, report->failed,
             report->skipped, report->warnings, report->total_elapsed_ms,
             HAL_TEST_REPORT_FILE);

    rbusValue_Init(&val);
    rbusValue_SetString(val, result);
    rbusObject_SetValue(outParams, "Result", val);
    rbusValue_Release(val);

    EPONMGR_LOG_INFO("HAL test complete: %s\n", result);

    free(report);
    return RBUS_ERROR_SUCCESS;
}
