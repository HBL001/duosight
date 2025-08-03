#!/bin/sh

# (c) 2025 Highland Biosciences
# Author: Dr Richard Day
# Email: richard_day@highlandbiosciences.com
#
# Summary:
#   Script to run diagnostic tests for DuoSight components.
#   Verifies both i2cUtils logic and MLX90640 sensor capture.

echo "=== DuoSight Self-Test Suite ==="

PASS=0

run_test() {
    BIN="$1"
    DESC="$2"

    if [ ! -x "$BIN" ]; then
        echo "❌ Error: $DESC not found or not executable ($BIN)"
        PASS=1
        return
    fi

    echo "--- Running: $DESC"
    "$BIN"
    RESULT=$?

    if [ "$RESULT" -eq 0 ]; then
        echo "✅ $DESC passed."
    elif [ "$RESULT" -eq 2 ]; then
        echo "⚠️  $DESC returned warnings (code 2)."
    else
        echo "❌ $DESC failed (code $RESULT)."
        PASS=1
    fi
}

run_test ./test_i2cUtils "I2C Utility Unit Test"
run_test ./test_mlx90640_reader "MLX90640 Sensor Self-Test"

echo "=== Self-Test Complete ==="
exit $PASS
