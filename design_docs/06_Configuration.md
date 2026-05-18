# Configuration and Deployment

## Persistent Configuration (PSM)

The EPON Manager stores all runtime-configurable parameters in the RDK
Persistent Storage Manager (PSM).  There is **no configuration file**; all
settings are read from PSM at startup and written back via the TR-181 SET
handlers.

### PSM Parameters

| PSM Key | Type | Default | Valid Range | Description |
|---------|------|---------|-------------|-------------|
| `dmsb.eponmanager.DpoeEnable` | bool | `false` | — | Enable DPoE (DOCSIS Provisioning of EPON) support |
| `dmsb.eponmanager.CacheTtlSeconds` | uint32 | `30` | 1–300 | TR-181 parameter cache time-to-live in seconds |
| `dmsb.eponmanager.StatsPollerEnabled` | bool | `false` | — | Enable periodic statistics polling thread |
| `dmsb.eponmanager.StatsPollerIntervalSeconds` | uint32 | `900` | 60–3600 | Statistics polling interval in seconds |

### TR-181 Mapping

The StatsPoller settings are exposed as read/write TR-181 parameters and
persisted to PSM on SET:

| TR-181 Parameter | PSM Key | Notes |
|------------------|---------|-------|
| `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.Enable` | `dmsb.eponmanager.StatsPollerEnabled` | Starts/stops poller thread on change |
| `Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.PollingInterval` | `dmsb.eponmanager.StatsPollerIntervalSeconds` | Applied immediately to running poller |

### Load / Save Flow

```
Startup:
  1. eponMgr_persistence_init_defaults()   → zero-fill + apply compiled defaults
  2. eponMgr_psm_init()                    → open rbus connection to PsmSsp
  3. eponMgr_persistence_load()            → read PSM keys; validate; keep default on failure

TR-181 SET:
  1. stats_poller_set_handler()            → validate, apply to running poller
  2. eponMgr_persistence_save()            → write all PSM keys back

Shutdown:
  1. eponMgr_psm_close()                   → release rbus handle
```

### Validation

| Parameter | Rule | On failure |
|-----------|------|-----------|
| `CacheTtlSeconds` | 1 ≤ value ≤ 300 | Log warning, use compiled default (30) |
| `StatsPollerIntervalSeconds` | 60 ≤ value ≤ 3600 | Log warning, use compiled default (900) |

---

## Systemd Service

**Source:** [`systemd/utils/rdkeponmanager.service`](../systemd/utils/rdkeponmanager.service)

```ini
[Unit]
Description=RDK EPON Manager service
After=rbus.service PsmSsp.service

[Service]
Type=forking
Environment="LOG4C_RCPATH=/etc"
EnvironmentFile=/etc/device.properties
WorkingDirectory=/usr/rdk/eponmanager
ExecStart=/usr/bin/epon_manager
ExecStop=/bin/sh -c 'echo "`date`: Stopping/Restarting RdkEponManager" >> ${PROCESS_RESTART_LOG}'
PIDFile=/var/tmp/epon_manager.pid
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Dependencies

| Service | Required | Purpose |
|---------|----------|---------|
| `rbus.service` | Yes | TR-181 data-model bus |
| `PsmSsp.service` | Yes | Persistent Storage Manager for configuration |

---

## Installation Locations

| Component | Path |
|-----------|------|
| Binary | `/usr/bin/epon_manager` |
| Working directory | `/usr/rdk/eponmanager/` |
| PID file | `/var/tmp/epon_manager.pid` |
| systemd unit | `/lib/systemd/system/rdkeponmanager.service` |

---

## Startup Sequence

```
1. systemd starts rdkeponmanager.service (after rbus + PsmSsp)
2. epon_manager forks, writes PID file
3. Init sequence:
     a. Logger init (LOG4C via /etc)
     b. RBus init → register TR-181 data elements
     c. PSM init → load persistent config
     d. HAL init → register callbacks (status, alarm, interface)
     e. Telemetry init → t2_init()
     f. Stats poller start (if StatsPollerEnabled == true)
     g. Controller event loop starts
4. Process ready
```

---

## Runtime Reconfiguration

All reconfiguration is done via TR-181 SET calls (no SIGHUP, no config file reload):

```bash
# Enable stats poller
dmcli eRT setv Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.Enable bool true

# Change polling interval to 5 minutes
dmcli eRT setv Device.Optical.Interface.1.X_RDK_EPON.StatsPoller.PollingInterval uint 300
```

Changes take effect immediately and are persisted across reboots via PSM.
