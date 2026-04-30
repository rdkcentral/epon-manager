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
 * @file eponMgr_harvester_priv.h
 * @brief Private prototypes shared between harvester source files.
 *
 * Not installed; only included by code under src/telemetry/.
 */

#ifndef EPONMGR_HARVESTER_PRIV_H
#define EPONMGR_HARVESTER_PRIV_H

#include "eponMgr_telemetry.h"
#include "eponMgr_data.h"

#include <avro.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Avro framing constants (compatible with rdk-xdslmanager pattern). */
#define EPON_HARV_MAGIC_NUMBER       0x8A
#define EPON_HARV_MAGIC_NUMBER_SIZE  1
#define EPON_HARV_SCHEMA_ID_LENGTH   32   /* 16-byte UUID + 16-byte MD5 */
#define EPON_HARV_WRITER_BUF_SIZE    (1024 * 16)

#define EPON_HARV_AVRO_SCHEMA_FILE   "/usr/ccsp/harvester/EponReport.avsc"
#define EPON_HARV_REPORT_NAME        "EPONTelemetryDiagnostics"
#define EPON_HARV_REPORT_SOURCE      "rdk-eponmanager"
#define EPON_HARV_CPE_TYPE_DEFAULT   "Gateway"

/* Avro field population (eponMgr_harvester_avro.c). */
int eponMgr_harv_fill_header(avro_value_t *report);
int eponMgr_harv_fill_cpe   (avro_value_t *report);
int eponMgr_harv_fill_data  (avro_value_t *report, eponMgr_data_t *data);

/* Frame & ship via WebPA (eponMgr_harvester_webpa.c). */
int eponMgr_harv_publish_frame(uint8_t *frame, size_t total_len);

#ifdef __cplusplus
}
#endif

#endif /* EPONMGR_HARVESTER_PRIV_H */
