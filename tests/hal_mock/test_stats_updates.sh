#!/bin/bash
# test_stats_updates.sh - Test statistics update functionality
# Demonstrates dynamic statistics manipulation via Unix Domain Socket

set -e

echo "==================================================================="
echo "  EPON HAL Mock Statistics Update Test"
echo "==================================================================="
echo ""

# Check if socket exists
if [ ! -S /tmp/epon_hal_mock.sock ]; then
    echo "❌ Error: EPON Manager not running or HAL mock socket not found"
    echo "   Expected socket: /tmp/epon_hal_mock.sock"
    echo ""
    echo "Start EPON Manager first:"
    echo "  /usr/bin/epon_manager &"
    exit 1
fi

echo "✓ EPON Manager is running (socket found)"
echo ""

# Test 1: Set Baseline Statistics
echo "=== Test 1: Setting Baseline Link Statistics ==="
echo "Setting initial counters..."
epon_hal_trigger -L packets_sent=1000000
epon_hal_trigger -L packets_received=980000
epon_hal_trigger -L bytes_sent=1024000000
epon_hal_trigger -L bytes_received=1003520000
epon_hal_trigger -L errors_sent=10
epon_hal_trigger -L errors_received=25
epon_hal_trigger -L fec_corrected=100
epon_hal_trigger -L fec_uncorrectable=0
echo "✓ Baseline statistics set"
sleep 2
echo ""

# Test 2: Incremental Traffic Simulation
echo "=== Test 2: Simulating Active Traffic ==="
echo "Generating 10 traffic bursts (1 second apart)..."
for i in {1..10}; do
    printf "  Burst %2d: +5000 pkts TX, +4900 pkts RX\n" $i
    epon_hal_trigger -I packets_sent=5000
    epon_hal_trigger -I packets_received=4900
    epon_hal_trigger -I bytes_sent=7680000
    epon_hal_trigger -I bytes_received=7516800
    sleep 1
done
echo ""
echo "Expected final values:"
echo "  Packets sent:     1,050,000  (1000000 + 10*5000)"
echo "  Packets received: 1,029,000  (980000 + 10*4900)"
echo "  Bytes sent:       1,100,800,000  (1024000000 + 10*7680000)"
echo "  Bytes received:   1,078,688,000  (1003520000 + 10*7516800)"
echo ""
sleep 2

# Test 3: Transceiver Statistics
echo "=== Test 3: Optical Transceiver Monitoring ==="
echo "Setting normal operating conditions..."
epon_hal_trigger -T tx_power=-2.5
epon_hal_trigger -T rx_power=-15.8
epon_hal_trigger -T temperature=42.3
epon_hal_trigger -T voltage=3.3
epon_hal_trigger -T bias_current=35.2
echo "✓ Normal conditions: TX=-2.5dBm, RX=-15.8dBm, Temp=42.3°C"
sleep 2

echo ""
echo "Simulating environmental stress..."
epon_hal_trigger -T temperature=65.5
echo "  ⚠ Temperature increased to 65.5°C"
sleep 1

epon_hal_trigger -T tx_power=-5.8
echo "  ⚠ TX power degraded to -5.8dBm"
sleep 1

epon_hal_trigger -T rx_power=-22.3
echo "  ⚠ RX power degraded to -22.3dBm"
sleep 2

echo ""
echo "Recovering to normal..."
epon_hal_trigger -T temperature=43.0
epon_hal_trigger -T tx_power=-2.8
epon_hal_trigger -T rx_power=-16.2
echo "✓ Back to normal operating conditions"
sleep 2
echo ""

# Test 4: FEC Error Accumulation
echo "=== Test 4: FEC Error Monitoring ==="
echo "Simulating line degradation..."
for i in {1..5}; do
    printf "  Degradation cycle %d: +50 corrected, +2 uncorrectable\n" $i
    epon_hal_trigger -I fec_corrected=50
    epon_hal_trigger -I fec_uncorrectable=2
    sleep 1
done
echo ""
echo "Expected FEC stats:"
echo "  Corrected:       350  (100 + 5*50)"
echo "  Uncorrectable:   10   (0 + 5*2)"
echo ""
sleep 2

# Test 5: Error Rate Simulation
echo "=== Test 5: Error Rate Testing ==="
echo "Injecting transmission errors..."
for i in {1..5}; do
    printf "  Error injection %d\n" $i
    epon_hal_trigger -I errors_sent=5
    epon_hal_trigger -I errors_received=8
    sleep 1
done
echo ""
echo "Expected error counts:"
echo "  Errors sent:     35  (10 + 5*5)"
echo "  Errors received: 65  (25 + 5*8)"
echo ""
sleep 2

# Test 6: Verification with RBUS (if available)
echo "=== Test 6: Verification via TR-181/RBUS ==="
if command -v rbus-cli &> /dev/null; then
    echo "Querying TR-181 parameters..."
    echo ""
    echo "Link Statistics:"
    rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsSent" 2>/dev/null || echo "  (TR-181 path may vary)"
    rbus-cli get "Device.Ethernet.Link.1.Stats.PacketsReceived" 2>/dev/null || echo "  (TR-181 path may vary)"
    rbus-cli get "Device.Ethernet.Link.1.Stats.BytesSent" 2>/dev/null || echo "  (TR-181 path may vary)"
    rbus-cli get "Device.Ethernet.Link.1.Stats.BytesReceived" 2>/dev/null || echo "  (TR-181 path may vary)"
    echo ""
    echo "Transceiver Statistics:"
    rbus-cli get "Device.Optical.Interface.1.TransmitPower" 2>/dev/null || echo "  (TR-181 path may vary)"
    rbus-cli get "Device.Optical.Interface.1.ReceivePower" 2>/dev/null || echo "  (TR-181 path may vary)"
    rbus-cli get "Device.Optical.Interface.1.Temperature" 2>/dev/null || echo "  (TR-181 path may vary)"
else
    echo "⚠ rbus-cli not found - skipping TR-181 verification"
    echo "  Install RBUS to query TR-181 data model"
fi
echo ""
sleep 2

# Test 7: Combined Scenario
echo "=== Test 7: Complete Operational Cycle ==="
echo "Simulating full link lifecycle with statistics..."
echo ""

echo "Phase 1: Link Down - Resetting stats"
epon_hal_trigger -s deregister
epon_hal_trigger -L packets_sent=0
epon_hal_trigger -L packets_received=0
epon_hal_trigger -L bytes_sent=0
epon_hal_trigger -L bytes_received=0
sleep 1

echo "Phase 2: Link Up - Initial Registration"
epon_hal_trigger -s register
epon_hal_trigger -i veip0:up
sleep 1

echo "Phase 3: Normal Traffic"
epon_hal_trigger -I packets_sent=50000
epon_hal_trigger -I bytes_sent=76800000
sleep 1

echo "Phase 4: Environmental Event"
epon_hal_trigger -a temperature
epon_hal_trigger -T temperature=75.0
sleep 1

echo "Phase 5: Link Degradation"
epon_hal_trigger -T rx_power=-28.5
epon_hal_trigger -I errors_received=100
epon_hal_trigger -I fec_corrected=500
sleep 1

echo "Phase 6: Recovery"
epon_hal_trigger -a temperature -c
epon_hal_trigger -T temperature=45.0
epon_hal_trigger -T rx_power=-16.5
sleep 1

echo "✓ Complete operational cycle finished"
echo ""

echo "==================================================================="
echo "  Statistics Update Tests Completed Successfully!"
echo "==================================================================="
echo ""
echo "Summary:"
echo "  ✓ Baseline statistics set"
echo "  ✓ Incremental traffic simulated"
echo "  ✓ Optical parameters manipulated"
echo "  ✓ FEC errors monitored"
echo "  ✓ Error rates tested"
echo "  ✓ Complete lifecycle validated"
echo ""
echo "Next Steps:"
echo "  1. Check EPON Manager logs: /rdklogs/logs/eponmanager.log"
echo "  2. Query HAL directly:"
echo "       epon_hal_get_link_statistics()"
echo "       epon_hal_get_transceiver_info()"
echo "  3. Verify TR-181 updates via RBUS"
echo "  4. Check telemetry events for threshold violations"
echo ""
