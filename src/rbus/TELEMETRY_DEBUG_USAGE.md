# EPON Manager – Telemetry Debug Trigger (TEMP – remove before PR merge)

Method path: `Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()`

---

## Batch modes

```sh
# Fire every event (all 34) with dummy context
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string all

# Fire all 15 alarm events as RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string all_alarms_raise

# Fire all 15 alarm events as CLEARED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string all_alarms_clear

# Fire all alarms RAISED then CLEARED (30 total)
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string all_alarms_both
```

---

## Single triggers – one per event

### §2.1 ONU Status

```sh
# 0 – EPON_TELEM_ONU_LOS
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 0

# 1 – EPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 1

# 2 – EPON_TELEM_ONU_REGISTRATION
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 2

# 3 – EPON_TELEM_ONU_DEREGISTRATION
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 3
```

### §2.2 Link / PHY Status

```sh
# 4 – EPON_TELEM_INTF_LINK_UP  (supply real interface name via Ifname)
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 4 Ifname string veip0

# 5 – EPON_TELEM_INTF_LINK_DOWN
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 5 Ifname string veip0

# 6 – EPON_TELEM_PHY_STATUS_UP
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 6

# 7 – EPON_TELEM_PHY_STATUS_DOWN
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 7
```

### §2.3 Standard 802.3ah Alarms

> Use `Raised uint32 1` for RAISED, `Raised uint32 0` for CLEARED.  
> Omit `Llid` or set `Llid uint32 65535` for device-wide (no LLID).

```sh
# 8 – EPON_TELEM_ALARM_STD_LOFI  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 8 Raised uint32 1

# 8 – EPON_TELEM_ALARM_STD_LOFI  CLEARED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 8 Raised uint32 0

# 9 – EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 9 Raised uint32 1

# 10 – EPON_TELEM_ALARM_STD_ERROR_FRAME  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 10 Raised uint32 1

# 11 – EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 11 Raised uint32 1

# 12 – EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 12 Raised uint32 1

# 13 – EPON_TELEM_ALARM_STD_OAM_SESSION_LOST  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 13 Raised uint32 1

# 14 – EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 14 Raised uint32 1
```

### §2.4 Vendor (DPoE) Alarms

```sh
# 15 – EPON_TELEM_ALARM_VENDOR_LOS  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 15 Raised uint32 1

# 16 – EPON_TELEM_ALARM_VENDOR_DYING_GASP  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 16 Raised uint32 1

# 17 – EPON_TELEM_ALARM_VENDOR_POWER_LOW  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 17 Raised uint32 1

# 18 – EPON_TELEM_ALARM_VENDOR_POWER_HIGH  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 18 Raised uint32 1

# 19 – EPON_TELEM_ALARM_VENDOR_TEMPERATURE  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 19 Raised uint32 1

# 20 – EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 20 Raised uint32 1

# 21 – EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 21 Raised uint32 1

# 22 – EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE  RAISED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 22 Raised uint32 1
```

> To trigger an alarm with a specific LLID (e.g. LLID 3):
> ```sh
> rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 15 Raised uint32 1 Llid uint32 3
> ```

### §2.5 System Lifecycle

```sh
# 23 – EPON_TELEM_SYSTEM_INIT_SUCCESS
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 23

# 24 – EPON_TELEM_SYSTEM_INIT_FAILURE
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 24

# 25 – EPON_TELEM_SYSTEM_SHUTDOWN
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 25

# 26 – EPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 26

# 27 – EPON_TELEM_SYSTEM_FACTORY_RESET
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 27

# 28 – EPON_TELEM_SYSTEM_ONU_RESET
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 28
```

### §2.6 Error Events

```sh
# 29 – EPON_TELEM_ERROR_HAL_CALL_FAILED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 29

# 30 – EPON_TELEM_ERROR_EVENT_QUEUE_FULL
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 30

# 31 – EPON_TELEM_ERROR_STATS_COLLECTION_FAILED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 31

# 32 – EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 32

# 33 – EPON_TELEM_ERROR_PSM_ACCESS_FAILED
rbuscli method_values "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()" Mode string single EventId uint32 33
```

---

## Shell script – trigger all events one by one

Save as `trigger_all_telemetry.sh` and run on the device:

```sh
#!/bin/sh

METHOD="Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()"
IFNAME="veip0"

fire() {
    echo ">>> EventId=$1  $2"
    rbuscli method_values "$METHOD" $3
}

fire  0 "ONU_LOS"                          "Mode string single EventId uint32 0"
fire  1 "ONU_DOWNSTREAM_SIGNAL_DETECTED"   "Mode string single EventId uint32 1"
fire  2 "ONU_REGISTRATION"                 "Mode string single EventId uint32 2"
fire  3 "ONU_DEREGISTRATION"               "Mode string single EventId uint32 3"
fire  4 "INTF_LINK_UP"                     "Mode string single EventId uint32 4 Ifname string $IFNAME"
fire  5 "INTF_LINK_DOWN"                   "Mode string single EventId uint32 5 Ifname string $IFNAME"
fire  6 "PHY_STATUS_UP"                    "Mode string single EventId uint32 6"
fire  7 "PHY_STATUS_DOWN"                  "Mode string single EventId uint32 7"
fire  8 "ALARM_STD_LOFI (RAISED)"          "Mode string single EventId uint32 8  Raised uint32 1"
fire  8 "ALARM_STD_LOFI (CLEARED)"         "Mode string single EventId uint32 8  Raised uint32 0"
fire  9 "ALARM_STD_ERROR_SYMBOL_PERIOD"    "Mode string single EventId uint32 9  Raised uint32 1"
fire 10 "ALARM_STD_ERROR_FRAME"            "Mode string single EventId uint32 10 Raised uint32 1"
fire 11 "ALARM_STD_ERROR_FRAME_PERIOD"     "Mode string single EventId uint32 11 Raised uint32 1"
fire 12 "ALARM_STD_ERROR_FRAME_SECONDS"    "Mode string single EventId uint32 12 Raised uint32 1"
fire 13 "ALARM_STD_OAM_SESSION_LOST"       "Mode string single EventId uint32 13 Raised uint32 1"
fire 14 "ALARM_STD_EQUIPMENT_FAILURE"      "Mode string single EventId uint32 14 Raised uint32 1"
fire 15 "ALARM_VENDOR_LOS"                 "Mode string single EventId uint32 15 Raised uint32 1"
fire 16 "ALARM_VENDOR_DYING_GASP"          "Mode string single EventId uint32 16 Raised uint32 1"
fire 17 "ALARM_VENDOR_POWER_LOW"           "Mode string single EventId uint32 17 Raised uint32 1"
fire 18 "ALARM_VENDOR_POWER_HIGH"          "Mode string single EventId uint32 18 Raised uint32 1"
fire 19 "ALARM_VENDOR_TEMPERATURE"         "Mode string single EventId uint32 19 Raised uint32 1"
fire 20 "ALARM_VENDOR_FEC_THRESHOLD"       "Mode string single EventId uint32 20 Raised uint32 1"
fire 21 "ALARM_VENDOR_LASER_BIAS_CURRENT"  "Mode string single EventId uint32 21 Raised uint32 1"
fire 22 "ALARM_VENDOR_SUPPLY_VOLTAGE"      "Mode string single EventId uint32 22 Raised uint32 1"
fire 23 "SYSTEM_INIT_SUCCESS"              "Mode string single EventId uint32 23"
fire 24 "SYSTEM_INIT_FAILURE"              "Mode string single EventId uint32 24"
fire 25 "SYSTEM_SHUTDOWN"                  "Mode string single EventId uint32 25"
fire 26 "SYSTEM_HAL_WRONG_PON_MODE"        "Mode string single EventId uint32 26"
fire 27 "SYSTEM_FACTORY_RESET"             "Mode string single EventId uint32 27"
fire 28 "SYSTEM_ONU_RESET"                 "Mode string single EventId uint32 28"
fire 29 "ERROR_HAL_CALL_FAILED"            "Mode string single EventId uint32 29"
fire 30 "ERROR_EVENT_QUEUE_FULL"           "Mode string single EventId uint32 30"
fire 31 "ERROR_STATS_COLLECTION_FAILED"    "Mode string single EventId uint32 31"
fire 32 "ERROR_RBUS_PUBLISH_FAILED"        "Mode string single EventId uint32 32"
fire 33 "ERROR_PSM_ACCESS_FAILED"          "Mode string single EventId uint32 33"

echo "Done."
```
