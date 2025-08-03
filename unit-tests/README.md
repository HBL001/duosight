# DuoSight Hardware Self-Test

This repository includes a test script, `run-tests.sh`, which runs functional tests against the key hardware peripherals on the Verdin i.MX8M Plus system.

---

## Purpose

The `run-tests.sh` script provides a standard way to verify that the board’s connected peripherals are accessible and responding correctly. It is intended for use during:

- Development bring-up
- Factory QC
- Field diagnostics
- Post-deployment verification

---

## ✅ What the Script Tests

| Test | Description |
|------|-------------|
| **I2C** (`test_i2cUtils`) | Verifies that the I2C bus is available and that basic communication routines (read/write) work or fail gracefully. |
| *(Future)* SPI | Check SPI bus presence and loopback or test device functionality |
| *(Future)* MLX90640 sensor | Attempt to read sensor metadata or image frame |
| *(Future)* GPIO | Toggle known GPIOs (e.g. backlight, DISP pin) and verify via state |
| *(Future)* Display | Check that an HDMI display is detected or that Weston is running |
| *(Future)* Network | Check Ethernet/IP address status, link up/down |

---

## 🚀 Running the Test

1. Make sure the test binary exists:
   ```bash
   make test_i2cUtils
   ```

2. Copy the binary and script to the target (e.g. via SCP):
   ```bash
   scp test_i2cUtils run-tests.sh torizon@verdin:~/duosight/
   ```

3. On the target device:
   ```bash
   cd ~/duosight/
   chmod +x run-tests.sh
   ./run-tests.sh
   ```

4. Observe the output:
   ```text
   === DuoSight Self-Test: I2C Layer ===
   Running test: ./test_i2cUtils
   ✅ Test passed.
   ```

---

## 💪 Notes

- The I2C test does **not** require a live sensor — it checks if the bus opens and fails gracefully.
- To add more hardware tests, simply create a test binary (e.g. `test_display`, `test_gpio`) and modify `run-tests.sh` to invoke it.

---

## 🧑‍💻 Maintainer

Dr Richard Day  
Highland Biosciences  
richard_day@highlandbiosciences.com

