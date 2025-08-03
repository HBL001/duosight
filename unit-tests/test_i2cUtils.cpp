/**
 * @file test_i2cUtils.cpp
 * @brief Basic functional test for duosight::I2cDevice class.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   This unit test verifies that the I2cDevice class handles basic
 *   error conditions gracefully (e.g. invalid device paths or addresses).
 *   Intended for use in test environments without real I2C devices.
 */

#include "i2cUtils.h"
#include <iostream>

int main() {
    using namespace duosight;

    // Change this to an intentionally missing bus to test failure handling
    const std::string device = "/dev/i2c-9";  // This is designed to go BOOM because we dont have i2c-9
    const uint8_t dummyAddr = 0x33;

    I2cDevice dev(device, dummyAddr);
    if (dev.isOpen()) {
        std::cerr << "[TEST] Unexpected: Opened nonexistent device!" << std::endl;
        return 1;
    } else {
        std::cout << "[TEST] Correctly failed to open nonexistent device." << std::endl;
    }

    // Optionally: try real bus and confirm it handles basic writeThenRead

    return 0;
}
