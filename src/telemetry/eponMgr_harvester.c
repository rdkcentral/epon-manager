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
 * @file eponMgr_harvester.c
 * @brief Harvester orchestration: schema load, periodic thread, public API.
 *
 * Modeled on rdk-xdslmanager's xdsl_report.c. Avro field encoding lives
 * in eponMgr_harvester_avro.c; framing & WebPA transport in
 * eponMgr_harvester_webpa.c.
 */

#include "eponMgr_telemetry.h"
#include "eponMgr_harvester_priv.h"
#include "eponMgr_logger.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* ====================================================================== *
 * Schema (loaded once)                                                     *
 * ====================================================================== */
static avro_schema_t       g_schema     = NULL;
static avro_value_iface_t *g_iface      = NULL;
static pthread_mutex_t     g_schema_mtx = PTHREAD_MUTEX_INITIALIZER;

static int read_file_to_buf(const char *path, char **out, size_t *out_len)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        EPONMGR_LOG_ERROR("harvester: cannot open schema %s\n", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    if (sz <= 0) { fclose(fp); return -1; }
    rewind(fp);

    char *buf = (char *)malloc((size_t)sz + 1);
    if (buf == NULL) { fclose(fp); return -1; }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    if (n != (size_t)sz) { free(buf); return -1; }
    buf[sz] = '\0';
    *out = buf;
    *out_len = (size_t)sz;
    return 0;
}

static int schema_load(void)
{
    pthread_mutex_lock(&g_schema_mtx);
    if (g_schema != NULL) {
        pthread_mutex_unlock(&g_schema_mtx);
        return 0;
    }

    char *json = NULL;
    size_t len = 0;
    if (read_file_to_buf(EPON_HARV_AVRO_SCHEMA_FILE, &json, &len) != 0) {
        pthread_mutex_unlock(&g_schema_mtx);
        return -1;
    }

    avro_schema_error_t err;
    if (avro_schema_from_json(json, len, &g_schema, &err) != 0) {
        EPONMGR_LOG_ERROR("harvester: avro_schema_from_json failed: %s\n",
                          avro_strerror());
        free(json);
        g_schema = NULL;
        pthread_mutex_unlock(&g_schema_mtx);
        return -1;
    }
    free(json);

    g_iface = avro_generic_class_from_schema(g_schema);
    if (g_iface == NULL) {
        EPONMGR_LOG_ERROR("harvester: avro_generic_class_from_schema failed\n");
        avro_schema_decref(g_schema);
        g_schema = NULL;
        pthread_mutex_unlock(&g_schema_mtx);
        return -1;
    }

    EPONMGR_LOG_INFO("harvester: loaded schema %s\n",
                     EPON_HARV_AVRO_SCHEMA_FILE);
    pthread_mutex_unlock(&g_schema_mtx);
    return 0;
}

static void schema_unload(void)
{
    pthread_mutex_lock(&g_schema_mtx);
    if (g_iface  != NULL) { avro_value_iface_decref(g_iface);  g_iface  = NULL; }
    if (g_schema != NULL) { avro_schema_decref     (g_schema); g_schema = NULL; }
    pthread_mutex_unlock(&g_schema_mtx);
}

static int schema_new_value(avro_value_t *out)
{
    pthread_mutex_lock(&g_schema_mtx);
    if (g_iface == NULL) {
        pthread_mutex_unlock(&g_schema_mtx);
        return -1;
    }
    int rc = avro_generic_value_new(g_iface, out);
    pthread_mutex_unlock(&g_schema_mtx);
    return (rc == 0) ? 0 : -1;
}

/* ====================================================================== *
 * Build & ship                                                             *
 * ====================================================================== */
static char            g_avro_buf[EPON_HARV_WRITER_BUF_SIZE];
static pthread_mutex_t g_publish_mtx = PTHREAD_MUTEX_INITIALIZER;

static int build_and_ship(eponMgr_data_t *data)
{
    if (data == NULL) return -1;
    if (schema_load() != 0) return -1;

    avro_value_t report;
    if (schema_new_value(&report) != 0) {
        EPONMGR_LOG_ERROR("harvester: failed to instantiate Avro record\n");
        return -1;
    }

    int rc = 0;
    rc |= eponMgr_harv_fill_header(&report);
    rc |= eponMgr_harv_fill_cpe   (&report);
    rc |= eponMgr_harv_fill_data  (&report, data);
    if (rc != 0) {
        EPONMGR_LOG_ERROR("harvester: failed to populate Avro record\n");
        avro_value_decref(&report);
        return -1;
    }

    pthread_mutex_lock(&g_publish_mtx);

    memset(g_avro_buf, 0, sizeof(g_avro_buf));
    g_avro_buf[0] = (char)EPON_HARV_MAGIC_NUMBER;

    avro_writer_t writer = avro_writer_memory(
        &g_avro_buf[EPON_HARV_MAGIC_NUMBER_SIZE + EPON_HARV_SCHEMA_ID_LENGTH],
        sizeof(g_avro_buf) -
            (EPON_HARV_MAGIC_NUMBER_SIZE + EPON_HARV_SCHEMA_ID_LENGTH));
    if (writer == NULL) {
        EPONMGR_LOG_ERROR("harvester: avro_writer_memory failed\n");
        pthread_mutex_unlock(&g_publish_mtx);
        avro_value_decref(&report);
        return -1;
    }

    if (avro_value_write(writer, &report) != 0) {
        EPONMGR_LOG_ERROR("harvester: avro_value_write failed: %s\n",
                          avro_strerror());
        avro_writer_free(writer);
        pthread_mutex_unlock(&g_publish_mtx);
        avro_value_decref(&report);
        return -1;
    }

    size_t avro_size = 0;
    avro_value_sizeof(&report, &avro_size);
    avro_writer_free(writer);
    avro_value_decref(&report);

    size_t total = (size_t)EPON_HARV_MAGIC_NUMBER_SIZE +
                   (size_t)EPON_HARV_SCHEMA_ID_LENGTH +
                   avro_size;

    int pub_rc = eponMgr_harv_publish_frame((uint8_t *)g_avro_buf, total);

    pthread_mutex_unlock(&g_publish_mtx);
    return pub_rc;
}

/* ====================================================================== *
 * Periodic thread                                                          *
 * ====================================================================== */
static pthread_t       g_thread;
static bool            g_thread_running = false;
static bool            g_shutdown       = false;
static uint32_t        g_interval_sec   = 0;
static bool            g_enabled        = false;
static pthread_mutex_t g_cv_mtx         = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_cv             = PTHREAD_COND_INITIALIZER;

static eponMgr_data_t *g_data_ref       = NULL;
static pthread_mutex_t g_data_ref_mtx   = PTHREAD_MUTEX_INITIALIZER;

static void *harvester_thread(void *arg)
{
    (void)arg;
    EPONMGR_LOG_INFO("harvester: thread started (interval=%us)\n",
                     g_interval_sec);

    pthread_mutex_lock(&g_cv_mtx);
    while (!g_shutdown) {
        if (g_interval_sec == 0) {
            pthread_cond_wait(&g_cv, &g_cv_mtx);
            continue;
        }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += g_interval_sec;

        int rc = pthread_cond_timedwait(&g_cv, &g_cv_mtx, &ts);
        if (g_shutdown) break;
        if (rc != 0 && rc != ETIMEDOUT) {
            EPONMGR_LOG_WARN("harvester: cond_timedwait err=%d\n", rc);
            continue;
        }
        bool want_publish = g_enabled;
        pthread_mutex_unlock(&g_cv_mtx);

        if (want_publish) {
            pthread_mutex_lock(&g_data_ref_mtx);
            eponMgr_data_t *d = g_data_ref;
            pthread_mutex_unlock(&g_data_ref_mtx);

            if (d != NULL && build_and_ship(d) != 0) {
                EPONMGR_LOG_WARN("harvester: periodic publish failed\n");
            }
        }
        pthread_mutex_lock(&g_cv_mtx);
    }
    pthread_mutex_unlock(&g_cv_mtx);
    EPONMGR_LOG_INFO("harvester: thread exiting\n");
    return NULL;
}

/* ====================================================================== *
 * Public API                                                               *
 * ====================================================================== */
int eponMgr_harvester_init(uint32_t interval_seconds, bool enabled)
{
    if (g_thread_running) {
        EPONMGR_LOG_WARN("harvester: already initialized\n");
        return 0;
    }
    if (schema_load() != 0) {
        EPONMGR_LOG_ERROR("harvester: schema load failed; init aborted\n");
        return -1;
    }
    g_interval_sec = interval_seconds;
    g_enabled      = enabled;
    g_shutdown     = false;
    if (pthread_create(&g_thread, NULL, harvester_thread, NULL) != 0) {
        EPONMGR_LOG_ERROR("harvester: pthread_create failed\n");
        return -1;
    }
    g_thread_running = true;
    return 0;
}

int eponMgr_harvester_publish_now(eponMgr_data_t *data)
{
    if (data == NULL) return -1;

    pthread_mutex_lock(&g_data_ref_mtx);
    g_data_ref = data;
    pthread_mutex_unlock(&g_data_ref_mtx);

    if (!g_enabled) return 0;
    return build_and_ship(data);
}

void eponMgr_harvester_cleanup(void)
{
    if (g_thread_running) {
        pthread_mutex_lock(&g_cv_mtx);
        g_shutdown = true;
        pthread_cond_signal(&g_cv);
        pthread_mutex_unlock(&g_cv_mtx);

        pthread_join(g_thread, NULL);
        g_thread_running = false;
    }
    schema_unload();
}
