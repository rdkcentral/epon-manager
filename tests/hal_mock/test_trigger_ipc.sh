#!/bin/bash
# Test script for EPON HAL Mock Trigger Utility

echo "==================================================================="
echo "  EPON HAL Mock IPC Test - Simulation Script"
echo "==================================================================="
echo ""

# Check if EPON Manager is running
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

# Test 1: ONU Bringup Sequence
echo "=== Test 1: ONU Bringup Sequence ==="
echo "Simulating ONU registration..."
echo ""

echo "  1. Downstream signal detected..."
epon_hal_trigger -s signal
sleep 1

echo "  2. ONU registering..."
epon_hal_trigger -s register
sleep 1

echo "  3. Interface veip0 coming up..."
epon_hal_trigger -i veip0:up
sleep 1

echo "✓ ONU bringup complete"
echo ""

# Test 2: Alarm Events
echo "=== Test 2: Alarm Event Testing ==="
echo "Triggering various alarms..."
echo ""

echo "  1. Temperature alarm (raised)..."
epon_hal_trigger -a temperature
sleep 1

echo "  2. Power low alarm (raised)..."
epon_hal_trigger -a power_low
sleep 1

echo "  3. Clearing temperature alarm..."
epon_hal_trigger -a temperature -c
sleep 1

echo "  4. Clearing power low alarm..."
epon_hal_trigger -a power_low -c
sleep 1

echo "✓ Alarm testing complete"
echo ""

# Test 3: Link Flap
echo "=== Test 3: Link Flap Simulation ==="
echo "Simulating rapid link up/down events..."
echo ""

for i in {1..3}; do
    echo "  Cycle $i: Down -> Up"
    epon_hal_trigger -i veip0:down -d 500
    epon_hal_trigger -i veip0:up -d 500
done

echo "✓ Link flap test complete"
echo ""

# Test 4: Critical Failure
echo "=== Test 4: Critical Failure Simulation ==="
echo "Simulating loss of signal..."
echo ""

echo "  1. LOS alarm raised..."
epon_hal_trigger -a los
sleep 1

echo "  2. Interface down..."
epon_hal_trigger -i veip0:down
sleep 1

echo "  3. ONU deregistering..."
epon_hal_trigger -s deregister
sleep 2

echo "  4. Recovering: LOS cleared..."
epon_hal_trigger -a los -c
sleep 1

echo "  5. Signal detected..."
epon_hal_trigger -s signal
sleep 1

echo "  6. Re-registering..."
epon_hal_trigger -s register
sleep 1

echo "  7. Interface up..."
epon_hal_trigger -i veip0:up
sleep 1

echo "✓ Failure recovery complete"
echo ""

echo "==================================================================="
echo "  All Tests Completed Successfully!"
echo "==================================================================="
echo ""
echo "Check EPON Manager logs for TR-181 updates and RDK Logger output:"
echo "  tail -f /opt/logs/epon_manager.log"
echo "  journalctl -u epon-manager -f"
echo ""
