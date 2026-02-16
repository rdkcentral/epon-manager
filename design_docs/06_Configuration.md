# Configuration and Deployment

## Configuration File

### Location
- **Primary**: `/etc/epon/epon_manager.conf`
- **Fallback**: `/usr/share/epon/epon_manager.conf.default`

### Format
INI-style configuration with sections and key-value pairs

### Example Configuration

```ini
[General]
# Component name for RDK
component_name=EponManager

# Log level: FATAL, ERROR, WARNING, INFO, DEBUG
log_level=INFO

# Enable statistics polling thread
enable_stats_polling=true

# Process name for identification
process_name=rdkeponmanager

[RBus]
# Bus type: rbus or dbus
bus_type=rbus

# Connection timeout in milliseconds
connection_timeout=5000

# Number of connection retry attempts
retry_attempts=3

# Retry delay in milliseconds
retry_delay=1000

[StatsPolling]
# Enable stats polling thread
enabled=true

# Polling interval in seconds (default: 900 = 15 minutes)
interval_seconds=900

# Enable telemetry reporting
telemetry_enabled=true

# Minimum polling interval (safety limit)
min_interval_seconds=60

[Cache]
# Maximum cache entries
max_entries=1000

# Default TTL in seconds
default_ttl_seconds=30

# Cache cleanup interval in seconds
cleanup_interval_seconds=60

# Enable cache statistics
enable_stats=true

[Telemetry]
# Enable T2 telemetry
t2_enabled=true

# Telemetry marker prefix
marker_prefix=EPON_

# Report interval in seconds
report_interval_seconds=300

# Enable event telemetry
enable_events=true

[HAL]
# HAL initialization timeout in milliseconds
init_timeout=10000

# Event queue size
event_queue_size=100

# HAL operation timeout in milliseconds
operation_timeout=5000

# Number of retry attempts for HAL operations
retry_attempts=3

[Logging]
# Log file location
log_file=/var/log/epon_manager.log

# Maximum log file size in MB
max_log_size_mb=10

# Number of log files to retain
log_file_count=5

# Enable console logging
enable_console=true

# Enable syslog
enable_syslog=true

[Performance]
# Event processing priority (0-99, higher is higher priority)
event_priority=50

# Enable performance monitoring
enable_monitoring=true

# Monitoring interval in seconds
monitoring_interval_seconds=60
```

---

## Environment Variables

### Supported Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `EPON_CONFIG_FILE` | Path to configuration file | `/etc/epon/epon_manager.conf` |
| `EPON_LOG_LEVEL` | Override log level | `INFO` |
| `EPON_BUS_TYPE` | Override bus type (rbus/dbus) | `rbus` |
| `EPON_DISABLE_STATS` | Disable stats polling (0/1) | `0` |
| `EPON_CACHE_SIZE` | Override cache size | `1000` |

### Example Usage

```bash
# Start with custom config
export EPON_CONFIG_FILE=/custom/path/epon.conf
/usr/bin/rdkeponmanager

# Start with debug logging
export EPON_LOG_LEVEL=DEBUG
/usr/bin/rdkeponmanager

# Disable stats polling
export EPON_DISABLE_STATS=1
/usr/bin/rdkeponmanager
```

---

## Deployment

### Build Dependencies

```
Required:
- gcc/g++ (C/C++ compiler)
- make
- pthread (POSIX threads)
- RDK common libraries
  - rbus
  - rdklogger
- EPON HAL library

Optional:
- T2 telemetry library (for telemetry support)
- systemd (for service management)
```

### Build Process

```bash
# Clone repository
git clone <repository-url>
cd rdkeponmanager

# Build
mkdir build
cd build
cmake ..
make

# Install
sudo make install
```

### Installation Locations

| Component | Location |
|-----------|----------|
| Binary | `/usr/bin/rdkeponmanager` |
| Configuration | `/etc/epon/epon_manager.conf` |
| Log file | `/var/log/epon_manager.log` |
| PID file | `/var/run/rdkeponmanager.pid` |
| systemd service | `/etc/systemd/system/rdkeponmanager.service` |

---

## Service Management

### systemd Service File

**File:** `/etc/systemd/system/rdkeponmanager.service`

```ini
[Unit]
Description=RDK EPON Manager
After=network.target rbus.service
Requires=rbus.service

[Service]
Type=simple
ExecStart=/usr/bin/rdkeponmanager
Restart=on-failure
RestartSec=5
User=root
Group=root
StandardOutput=journal
StandardError=journal

# Resource limits
LimitNOFILE=1024
LimitNPROC=512

# Watchdog
WatchdogSec=60s

[Install]
WantedBy=multi-user.target
```

### Service Commands

```bash
# Enable service
sudo systemctl enable rdkeponmanager

# Start service
sudo systemctl start rdkeponmanager

# Stop service
sudo systemctl stop rdkeponmanager

# Restart service
sudo systemctl restart rdkeponmanager

# Check status
sudo systemctl status rdkeponmanager

# View logs
sudo journalctl -u rdkeponmanager -f
```

---

## Startup Sequence

### Boot Process

```
1. System Boot
    
2. Network initialization
    
3. RBus service starts
    
4. RdkEponManager service starts
    
5. EponManager initializes
     Load configuration
     Initialize logger
     Start RBus thread
     Start telemetry
     Initialize HAL
     Start event listener
     Start stats poller
    
6. System ready
```

### Dependencies

**Required Services:**
- Network
- RBus/DBus
- System logger

**Optional Services:**
- T2 telemetry
- WanManager (for full functionality)

---

## Runtime Configuration

### Configuration Reload

Signal-based reload (without restart):

```bash
# Send SIGHUP to reload configuration
sudo pkill -HUP rdkeponmanager

# Or via systemd
sudo systemctl reload rdkeponmanager
```

### Dynamic Log Level Change

```bash
# Via RBus command
dmcli eRT setv Device.EPON.X_RDK_LogLevel string DEBUG

# Via configuration file (requires reload)
# Edit /etc/epon/epon_manager.conf
# Then: sudo systemctl reload rdkeponmanager
```

---

## Monitoring and Debugging

### Log Files

```bash
# Main log file
tail -f /var/log/epon_manager.log

# System logs
journalctl -u rdkeponmanager -f

# Filter by log level
journalctl -u rdkeponmanager -p err -f
```

### Debug Mode

Enable debug logging in configuration:

```ini
[General]
log_level=DEBUG

[Logging]
enable_console=true
```

Restart service:
```bash
sudo systemctl restart rdkeponmanager
```

### Performance Monitoring

Check resource usage:

```bash
# CPU and memory usage
top -p $(pgrep rdkeponmanager)

# Detailed process info
ps aux | grep rdkeponmanager

# Thread count
ps -eLf | grep rdkeponmanager | wc -l
```

---

## Troubleshooting

### Common Issues

#### 1. Service fails to start

**Symptoms:** Service exits immediately after start

**Diagnosis:**
```bash
sudo journalctl -u rdkeponmanager -n 50
```

**Solutions:**
- Check configuration file syntax
- Verify RBus service is running
- Check HAL library availability
- Review file permissions

#### 2. High CPU usage

**Symptoms:** Process consuming excessive CPU

**Diagnosis:**
```bash
top -p $(pgrep rdkeponmanager)
strace -p $(pgrep rdkeponmanager)
```

**Solutions:**
- Increase stats polling interval
- Check for HAL event storms
- Review cache configuration

#### 3. Memory leaks

**Symptoms:** Memory usage grows over time

**Diagnosis:**
```bash
valgrind --leak-check=full /usr/bin/rdkeponmanager
```

**Solutions:**
- Update to latest version
- Reduce cache size
- Check HAL library for leaks

#### 4. Events not processed

**Symptoms:** Link status not updated in WanManager

**Diagnosis:**
```bash
# Check event listener thread
ps -eLf | grep rdkeponmanager
# Check logs for event processing
grep "Event" /var/log/epon_manager.log
```

**Solutions:**
- Verify HAL is generating events
- Check RBus connectivity
- Review event queue configuration

---

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo cp /etc/epon/epon_manager.conf /etc/epon/epon_manager.conf.backup

# Restore configuration
sudo cp /etc/epon/epon_manager.conf.backup /etc/epon/epon_manager.conf
sudo systemctl restart rdkeponmanager
```

### Factory Reset

```bash
# Stop service
sudo systemctl stop rdkeponmanager

# Restore default configuration
sudo cp /usr/share/epon/epon_manager.conf.default /etc/epon/epon_manager.conf

# Clear logs
sudo rm /var/log/epon_manager.log*

# Start service
sudo systemctl start rdkeponmanager
```

---

## Security Considerations

### File Permissions

```bash
# Configuration file
chmod 644 /etc/epon/epon_manager.conf
chown root:root /etc/epon/epon_manager.conf

# Binary
chmod 755 /usr/bin/rdkeponmanager
chown root:root /usr/bin/rdkeponmanager

# Log directory
chmod 755 /var/log
chown root:root /var/log/epon_manager.log
```

### Process Security

- Run as root (required for HAL access)
- Resource limits enforced via systemd
- Watchdog enabled for crash recovery

---

## Upgrade and Migration

### Version Upgrade

```bash
# Stop service
sudo systemctl stop rdkeponmanager

# Backup current version
sudo cp /usr/bin/rdkeponmanager /usr/bin/rdkeponmanager.old

# Install new version
sudo cp rdkeponmanager /usr/bin/rdkeponmanager

# Backup configuration
sudo cp /etc/epon/epon_manager.conf /etc/epon/epon_manager.conf.backup

# Review new configuration options
# (if configuration format changed)

# Start service
sudo systemctl start rdkeponmanager

# Verify operation
sudo systemctl status rdkeponmanager
```

### Rollback

```bash
# Stop service
sudo systemctl stop rdkeponmanager

# Restore old version
sudo cp /usr/bin/rdkeponmanager.old /usr/bin/rdkeponmanager

# Restore configuration if needed
sudo cp /etc/epon/epon_manager.conf.backup /etc/epon/epon_manager.conf

# Start service
sudo systemctl start rdkeponmanager
```

---

**Document Version:** 1.0  
**Last Updated:** December 2, 2025
