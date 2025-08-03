#!/bin/sh

# (c) 2025 Highland Biosciences
# Author: Dr Richard Day
# Email: richard_day@highlandbiosciences.com
#
# Summary:
#   Script to run diagnostic tests for DuoSight components.
#   Currently verifies i2cUtils behavior.

echo "=== DuoSight Self-Test: I2C Layer ==="

TEST_BIN="./test_i2cUtils"

if [ ! -x "$TEST_BIN" ]; then
    echo "❌ Error: $TEST_BIN not found or not executable."
    exit 1
fi

echo "Running test: $TEST_BIN"
"$TEST_BIN"
RESULT=$?

if [ "$RESULT" -eq 0 ]; then
    echo "✅ Test passed."
else
    echo "❌ Test failed with code $RESULT."
fi

exit $RESULT

