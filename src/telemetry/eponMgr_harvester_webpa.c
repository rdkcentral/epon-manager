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
 * @file eponMgr_harvester_webpa.c
 * @brief Frame [MAGIC|UUID|MD5|avro] and ship via libparodus / WebPA.
 *
 * Frame format (matches rdk-xdslmanager so the existing Kestrel pipeline
 * can ingest the report):
 *   byte 0       : MAGIC = 0x8A
 *   bytes 1..16  : Schema UUID (constant per schema version)
 *   bytes 17..32 : MD5 of the deployed .avsc file (computed once)
 *   bytes 33..N  : Avro binary payload
 *
 * Without libparodus the routine logs the framed size and returns OK,
 * which keeps the rest of the pipeline functional for hal_mock tests.
 */

#include "eponMgr_harvester_priv.h"
#include "eponMgr_logger.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_LIBCRYPTO
#include <openssl/md5.h>
#endif

#ifdef HAVE_LIBPARODUS
#include <libparodus.h>
#include <wrp-c.h>
#endif

/*
 * Schema UUID is a stable per-schema-version constant; bump on schema
 * changes. Value derived from the literal
 * "rdk-eponmanager:EPONTelemetryDiagnostics:v1".
 */
static const uint8_t k_schema_uuid[16] = {
    0xe5, 0x0f, 0xab, 0xde, 0x73, 0x21, 0x4e, 0x9a,
    0x8c, 0x44, 0x6d, 0x2e, 0x1f, 0xb3, 0x77, 0x05,
};

static pthread_mutex_t g_mtx        = PTHREAD_MUTEX_INITIALIZER;
static bool            g_md5_ready  = false;
static uint8_t         g_schema_md5[16];
#ifdef HAVE_LIBPARODUS
static libpd_instance_t g_parodus   = NULL;
#endif

static void compute_schema_md5(void)
{
    pthread_mutex_lock(&g_mtx);
    if (g_md5_ready) {
        pthread_mutex_unlock(&g_mtx);
        return;
    }
    memset(g_schema_md5, 0, sizeof(g_schema_md5));

#ifdef HAVE_LIBCRYPTO
    FILE *fp = fopen(EPON_HARV_AVRO_SCHEMA_FILE, "rb");
    if (fp != NULL) {
        MD5_CTX ctx;
        MD5_Init(&ctx);
        uint8_t buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
            MD5_Update(&ctx, buf, n);
        }
        fclose(fp);
        MD5_Final(g_schema_md5, &ctx);
    } else {
        EPONMGR_LOG_WARN("harvester: schema md5 fopen failed; using zeros\n");
    }
#else
    EPONMGR_LOG_WARN("harvester: libcrypto not present; md5 left zero\n");
#endif

    g_md5_ready = true;
    pthread_mutex_unlock(&g_mtx);
}

#ifdef HAVE_LIBPARODUS
static int parodus_ensure_connected(void)
{
    if (g_parodus != NULL) return 0;

    libpd_cfg_t cfg = {
        .service_name           = "EPON",
        .receive                = false,
        .keepalive_timeout_secs = 0,
        .parodus_url            = "tcp://127.0.0.1:6666",
        .client_url             = "tcp://127.0.0.1:6667",
    };
    int rc = libparodus_init(&g_parodus, &cfg);
    if (rc != 0) {
        EPONMGR_LOG_WARN("harvester: libparodus_init failed: %d\n", rc);
        g_parodus = NULL;
        return -1;
    }
    EPONMGR_LOG_INFO("harvester: parodus client initialized\n");
    return 0;
}
#endif

int eponMgr_harv_publish_frame(uint8_t *frame, size_t total_len)
{
    if (frame == NULL ||
        total_len <= (size_t)(EPON_HARV_MAGIC_NUMBER_SIZE +
                              EPON_HARV_SCHEMA_ID_LENGTH)) {
        return -1;
    }

    compute_schema_md5();

    memcpy(&frame[EPON_HARV_MAGIC_NUMBER_SIZE],
           k_schema_uuid, sizeof(k_schema_uuid));
    memcpy(&frame[EPON_HARV_MAGIC_NUMBER_SIZE + sizeof(k_schema_uuid)],
           g_schema_md5, sizeof(g_schema_md5));

    EPONMGR_LOG_INFO("harvester: framed %zu byte report\n", total_len);

#ifdef HAVE_LIBPARODUS
    if (parodus_ensure_connected() != 0) return -1;

    char trans_id[64];
    snprintf(trans_id, sizeof(trans_id), "epon-%lx",
             (unsigned long)time(NULL));

    wrp_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_type                 = WRP_MSG_TYPE__EVENT;
    msg.u.event.source           = "EPON";
    msg.u.event.dest             = "event:raw.kestrel.reports.EponReport";
    msg.u.event.content_type     = "avro/binary";
    msg.u.event.transaction_uuid = trans_id;
    msg.u.event.payload          = (void *)frame;
    msg.u.event.payload_size     = total_len;

    int rc = libparodus_send(g_parodus, &msg);
    if (rc != 0) {
        EPONMGR_LOG_WARN("harvester: libparodus_send failed: %d\n", rc);
        return -1;
    }
    EPONMGR_LOG_INFO("harvester: report dispatched to WebPA\n");
    return 0;
#else
    EPONMGR_LOG_DEBUG("harvester: (no-libparodus) suppressed dispatch\n");
    return 0;
#endif
}
