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
 * @file eponMgr_hal_test.h
 * @brief EPON HAL Validation Test Suite
 *
 * Provides a comprehensive test suite for validating all EPON HAL APIs
 * on actual ONU hardware. Tests are triggered via an RBUS method call:
 *
 *   rbusMethod_Invoke(handle, "Device.Optical.Interface.1.X_RDK_EPON.RunHALTest", ...)
 *
 * Results are logged to the RDK log and a dedicated test report file:
 *   /rdklogs/logs/epon_hal_test_report.txt
 */

#ifndef EPONMGR_HAL_TEST_H
#define EPONMGR_HAL_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include <rbus/rbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Test report output file path */
#define HAL_TEST_REPORT_FILE "/rdklogs/logs/epon_hal_test_report.txt"

/** @brief Maximum length for a single test case name */
#define HAL_TEST_NAME_LEN 128

/** @brief Maximum length for a test detail/reason string */
#define HAL_TEST_DETAIL_LEN 512

/** @brief Maximum number of individual test cases */
#define HAL_TEST_MAX_CASES 64

/**
 * @brief Test result status for a single test case
 */
typedef enum {
    HAL_TEST_PASS = 0,      /**< Test passed */
    HAL_TEST_FAIL,          /**< Test failed */
    HAL_TEST_SKIP,          /**< Test skipped (not applicable or precondition not met) */
    HAL_TEST_WARN           /**< Test passed with warning (unexpected but not failure) */
} hal_test_status_t;

/**
 * @brief Individual test case result
 */
typedef struct {
    char name[HAL_TEST_NAME_LEN];       /**< Test case name */
    hal_test_status_t status;           /**< Result status */
    char detail[HAL_TEST_DETAIL_LEN];   /**< Details / reason */
    char method[HAL_TEST_DETAIL_LEN];   /**< Testing method description */
    double elapsed_ms;                  /**< Execution time in milliseconds */
} hal_test_case_result_t;

/**
 * @brief Overall test report summary
 */
typedef struct {
    uint32_t total;     /**< Total test cases executed */
    uint32_t passed;    /**< Number of tests passed */
    uint32_t failed;    /**< Number of tests failed */
    uint32_t skipped;   /**< Number of tests skipped */
    uint32_t warnings;  /**< Number of tests with warnings */
    double total_elapsed_ms; /**< Total execution time */
    hal_test_case_result_t cases[HAL_TEST_MAX_CASES]; /**< Individual results */
} hal_test_report_t;

/**
 * @brief Run the full EPON HAL validation test suite
 *
 * Executes all HAL API tests on the live hardware and produces a report.
 * Safe to call while the EPON Manager is running - uses the already-initialized HAL.
 *
 * @param[out] report  Pointer to report structure to fill (caller-allocated)
 * @param[in]  include_destructive  If true, run destructive tests (reset_onu, factory_reset)
 *                                  that will cause service disruption. Default: false.
 * @return 0 on success (report generated), -1 on error (could not run tests)
 */
int eponMgr_hal_test_run_all(hal_test_report_t *report, bool include_destructive);

/**
 * @brief Write the test report to the report file and RDK logs
 *
 * @param report  Pointer to completed report
 * @return 0 on success, -1 on error
 */
int eponMgr_hal_test_write_report(const hal_test_report_t *report);

/**
 * @brief RBUS method handler for triggering HAL tests
 *
 * Registered as: Device.Optical.Interface.1.X_RDK_EPON.RunHALTest()
 * Can be invoked via: rbusMethod_Invoke(handle, "..RunHALTest", inParams, &outParams)
 *
 * Output parameters set on outParams:
 *   - "TotalTests"   (uint32)
 *   - "Passed"       (uint32)
 *   - "Failed"       (uint32)
 *   - "Skipped"      (uint32)
 *   - "ReportFile"   (string) - path to detailed report
 *
 * Input parameters (optional, via inParams):
 *   - "IncludeDestructive" (bool) - if true, run reset_onu and factory_reset tests
 *
 * @return RBUS_ERROR_SUCCESS on success
 */
rbusError_t eponMgr_hal_test_rbus_handler(rbusHandle_t handle,
                                          char const *methodName,
                                          rbusObject_t inParams,
                                          rbusObject_t outParams,
                                          rbusMethodAsyncHandle_t asyncHandle);

#ifdef __cplusplus
}
#endif

#endif /* EPONMGR_HAL_TEST_H */
