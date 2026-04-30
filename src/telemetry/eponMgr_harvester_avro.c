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
 * @file eponMgr_harvester_avro.c
 * @brief Avro encoding for the EPONTelemetryDiagnostics record.
 *
 * Three sub-records are populated:
 *   - header (Kestrel CoreHeader): timestamp / uuid / source
 *   - cpe_id (Kestrel CPEIdentifier): mac_address / cpe_type
 *   - data   (EPONTelemetryData)  : 32 fields drawn from eponMgr_data_t
 *
 * Float optical/transceiver values from the HAL are converted to the
 * Dbm1000 / mC / mV / uA integer encodings expected by the schema by
 * multiplication by 1000 and rounding.
 */

#include "eponMgr_harvester_priv.h"
#include "eponMgr_logger.h"
#include "eponMgr_data.h"
#include "eponMgr_onu_state.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GW_MAC_LEN 6

/* ====================================================================== *
 * Generic helpers                                                          *
 * ====================================================================== */

static int union_select_nonnull(avro_value_t *u, avro_value_t *out)
{
    if (avro_value_set_branch(u, 1, out) != 0) {
        EPONMGR_LOG_ERROR("harvester: union set_branch failed: %s\n",
                          avro_strerror());
        return -1;
    }
    return 0;
}

static int set_long_field(avro_value_t *parent, const char *name, int64_t v)
{
    avro_value_t f, inner;
    if (avro_value_get_by_name(parent, name, &f, NULL) != 0) return -1;
    if (avro_value_set_branch(&f, 1, &inner) != 0) return -1;
    return avro_value_set_long(&inner, v);
}

static int set_int_field(avro_value_t *parent, const char *name, int32_t v)
{
    avro_value_t f, inner;
    if (avro_value_get_by_name(parent, name, &f, NULL) != 0) return -1;
    if (avro_value_set_branch(&f, 1, &inner) != 0) return -1;
    return avro_value_set_int(&inner, v);
}

static int set_string_field(avro_value_t *parent, const char *name,
                            const char *v)
{
    avro_value_t f, inner;
    if (avro_value_get_by_name(parent, name, &f, NULL) != 0) return -1;
    if (avro_value_set_branch(&f, 1, &inner) != 0) return -1;
    return avro_value_set_string(&inner, v);
}

static int set_null_field(avro_value_t *parent, const char *name)
{
    avro_value_t f, inner;
    if (avro_value_get_by_name(parent, name, &f, NULL) != 0) return -1;
    if (avro_value_set_branch(&f, 0, &inner) != 0) return -1;
    return avro_value_set_null(&inner);
}

static int32_t f_to_milli(float v)
{
    return (int32_t)lroundf(v * 1000.0f);
}

static void gen_uuid_v4(uint8_t out[16])
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, out, 16);
        close(fd);
        if (n == 16) {
            out[6] = (out[6] & 0x0F) | 0x40;
            out[8] = (out[8] & 0x3F) | 0x80;
            return;
        }
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    for (int i = 0; i < 16; i++) {
        out[i] = (uint8_t)((ts.tv_nsec ^ (i * 0x9E3779B1u)) & 0xFF);
    }
    out[6] = (out[6] & 0x0F) | 0x40;
    out[8] = (out[8] & 0x3F) | 0x80;
}

static int read_gateway_mac(uint8_t out[GW_MAC_LEN])
{
    static const char *kCandidates[] = { "erouter0", "brlan0", "eth0", NULL };
    for (int i = 0; kCandidates[i] != NULL; i++) {
        char path[128];
        snprintf(path, sizeof(path),
                 "/sys/class/net/%s/address", kCandidates[i]);
        FILE *fp = fopen(path, "r");
        if (fp == NULL) continue;

        unsigned int b[GW_MAC_LEN];
        int n = fscanf(fp, "%x:%x:%x:%x:%x:%x",
                       &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
        fclose(fp);
        if (n == GW_MAC_LEN) {
            for (int k = 0; k < GW_MAC_LEN; k++) out[k] = (uint8_t)b[k];
            return 0;
        }
    }
    return -1;
}

/* ====================================================================== *
 * Header (Kestrel CoreHeader)                                              *
 * ====================================================================== */
int eponMgr_harv_fill_header(avro_value_t *report)
{
    avro_value_t header;
    if (avro_value_get_by_name(report, "header", &header, NULL) != 0)
        return -1;

    /* timestamp */
    {
        avro_value_t f, inner;
        if (avro_value_get_by_name(&header, "timestamp", &f, NULL) != 0)
            return -1;
        if (union_select_nonnull(&f, &inner) != 0) return -1;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        int64_t millis = (int64_t)ts.tv_sec * 1000LL +
                         (int64_t)(ts.tv_nsec / 1000000);
        if (avro_value_set_long(&inner, millis) != 0) return -1;
    }

    /* uuid (fixed[16]) */
    {
        avro_value_t f, inner;
        if (avro_value_get_by_name(&header, "uuid", &f, NULL) != 0)
            return -1;
        if (union_select_nonnull(&f, &inner) != 0) return -1;
        uint8_t uuid[16];
        gen_uuid_v4(uuid);
        if (avro_value_set_fixed(&inner, uuid, sizeof(uuid)) != 0)
            return -1;
    }

    /* source */
    {
        avro_value_t f, inner;
        if (avro_value_get_by_name(&header, "source", &f, NULL) != 0)
            return -1;
        if (union_select_nonnull(&f, &inner) != 0) return -1;
        if (avro_value_set_string(&inner, EPON_HARV_REPORT_SOURCE) != 0)
            return -1;
    }
    return 0;
}

/* ====================================================================== *
 * CPE identifier                                                           *
 * ====================================================================== */
int eponMgr_harv_fill_cpe(avro_value_t *report)
{
    avro_value_t cpe;
    if (avro_value_get_by_name(report, "cpe_id", &cpe, NULL) != 0)
        return -1;

    /* mac_address (fixed[6]) */
    {
        avro_value_t f, inner;
        if (avro_value_get_by_name(&cpe, "mac_address", &f, NULL) != 0)
            return -1;
        uint8_t mac[GW_MAC_LEN];
        if (read_gateway_mac(mac) == 0) {
            if (union_select_nonnull(&f, &inner) != 0) return -1;
            if (avro_value_set_fixed(&inner, mac, sizeof(mac)) != 0)
                return -1;
        } else {
            avro_value_t nul;
            if (avro_value_set_branch(&f, 0, &nul) != 0) return -1;
            (void)avro_value_set_null(&nul);
        }
    }

    /* cpe_type */
    {
        avro_value_t f, inner;
        if (avro_value_get_by_name(&cpe, "cpe_type", &f, NULL) != 0)
            return -1;
        if (union_select_nonnull(&f, &inner) != 0) return -1;
        if (avro_value_set_string(&inner, EPON_HARV_CPE_TYPE_DEFAULT) != 0)
            return -1;
    }
    return 0;
}

/* ====================================================================== *
 * EPONTelemetryData                                                        *
 * ====================================================================== */
int eponMgr_harv_fill_data(avro_value_t *report, eponMgr_data_t *data)
{
    if (data == NULL) return -1;

    avro_value_t d;
    if (avro_value_get_by_name(report, "data", &d, NULL) != 0) return -1;

    const epon_hal_link_stats_t        *ls =
        eponMgr_data_get_link_stats(data);
    const epon_hal_transceiver_stats_t *ts =
        eponMgr_data_get_transceiver_stats(data);

    /* --- link / packet counters ----------------------------------- */
    if (ls != NULL) {
        set_long_field(&d, "BytesSent",                 (int64_t)ls->bytes_sent);
        set_long_field(&d, "BytesReceived",             (int64_t)ls->bytes_received);
        set_long_field(&d, "PacketsSent",               (int64_t)ls->packets_sent);
        set_long_field(&d, "PacketsReceived",           (int64_t)ls->packets_received);
        set_long_field(&d, "ErrorsSent",                (int64_t)ls->errors_sent);
        set_long_field(&d, "ErrorsReceived",            (int64_t)ls->errors_received);
        set_long_field(&d, "DiscardPacketsSent",        (int64_t)ls->discard_packets_sent);
        set_long_field(&d, "DiscardPacketsReceived",    (int64_t)ls->discard_packets_received);
        set_long_field(&d, "BroadcastPacketsSent",      (int64_t)ls->broadcast_packets_sent);
        set_long_field(&d, "BroadcastPacketsReceived",  (int64_t)ls->broadcast_packets_received);
        set_long_field(&d, "MulticastPacketsSent",      (int64_t)ls->multicast_packets_sent);
        set_long_field(&d, "MulticastPacketsReceived",  (int64_t)ls->multicast_packets_received);
        set_int_field (&d, "MaxBitRate",                (int32_t)ls->max_bit_rate);
        set_long_field(&d, "FECCorrected",              (int64_t)ls->fec_corrected);
        set_long_field(&d, "FECUncorrectable",          (int64_t)ls->fec_uncorrectable);
        set_long_field(&d, "RangingResyncs",            (int64_t)ls->ranging_resyncs);
        set_long_field(&d, "MACResets",                 (int64_t)ls->mac_resets);
    } else {
        set_null_field(&d, "BytesSent");
        set_null_field(&d, "BytesReceived");
        set_null_field(&d, "PacketsSent");
        set_null_field(&d, "PacketsReceived");
        set_null_field(&d, "ErrorsSent");
        set_null_field(&d, "ErrorsReceived");
        set_null_field(&d, "DiscardPacketsSent");
        set_null_field(&d, "DiscardPacketsReceived");
        set_null_field(&d, "BroadcastPacketsSent");
        set_null_field(&d, "BroadcastPacketsReceived");
        set_null_field(&d, "MulticastPacketsSent");
        set_null_field(&d, "MulticastPacketsReceived");
        set_null_field(&d, "MaxBitRate");
        set_null_field(&d, "FECCorrected");
        set_null_field(&d, "FECUncorrectable");
        set_null_field(&d, "RangingResyncs");
        set_null_field(&d, "MACResets");
    }

    /* HAL does not expose unicast / BER counters yet. */
    set_null_field(&d, "UnicastPacketsSent");
    set_null_field(&d, "UnicastPacketsReceived");
    set_null_field(&d, "BER");

    /* --- transceiver / optical (float -> int x1000) --------------- */
    if (ts != NULL) {
        set_int_field(&d, "TransmitOpticalLevel",        f_to_milli(ts->transmit_optical_level));
        set_int_field(&d, "OpticalSignalLevel",          f_to_milli(ts->optical_signal_level));
        set_int_field(&d, "LowerOpticalThreshold",       f_to_milli(ts->lower_optical_threshold));
        set_int_field(&d, "UpperOpticalThreshold",       f_to_milli(ts->upper_optical_threshold));
        set_int_field(&d, "LowerTransmitPowerThreshold", f_to_milli(ts->lower_transmit_power_threshold));
        set_int_field(&d, "UpperTransmitPowerThreshold", f_to_milli(ts->upper_transmit_power_threshold));
        set_int_field(&d, "TransceiverTemperature",      f_to_milli(ts->temperature));
        set_int_field(&d, "SupplyVoltage",               f_to_milli(ts->supply_voltage));
        set_int_field(&d, "LaserBiasCurrent",            f_to_milli(ts->bias_current));
    } else {
        set_null_field(&d, "TransmitOpticalLevel");
        set_null_field(&d, "OpticalSignalLevel");
        set_null_field(&d, "LowerOpticalThreshold");
        set_null_field(&d, "UpperOpticalThreshold");
        set_null_field(&d, "LowerTransmitPowerThreshold");
        set_null_field(&d, "UpperTransmitPowerThreshold");
        set_null_field(&d, "TransceiverTemperature");
        set_null_field(&d, "SupplyVoltage");
        set_null_field(&d, "LaserBiasCurrent");
    }

    /* --- ONU state-derived fields --------------------------------- */
    eponMgr_onu_state_t *st = data->onu_state;
    if (st != NULL) {
        if (st->link_info_valid) {
            set_string_field(&d, "OperationalMode", st->link_info.mode);
            set_int_field   (&d, "EncryptionMode",
                             (int32_t)st->link_info.encryption);
        } else {
            set_null_field(&d, "OperationalMode");
            set_null_field(&d, "EncryptionMode");
        }
        set_int_field(&d, "ONUStatus", (int32_t)st->current_status);
    } else {
        set_null_field(&d, "OperationalMode");
        set_null_field(&d, "EncryptionMode");
        set_null_field(&d, "ONUStatus");
    }

    return 0;
}
