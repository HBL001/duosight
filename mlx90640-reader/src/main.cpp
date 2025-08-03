/**
 * @file main.cpp
 * @brief Developer test for MLX90640Reader (WIP thermal sensor interface).
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Basic CLI test harness for MLX90640Reader.
 *   Initializes the sensor, captures one thermal frame,
 *   and prints min/max/avg temperature for validation.
 *   Intended for work-in-progress debugging only.
 */

#include "MLX90640Reader.h"
#include <vector>
#include <iostream>

int main() {
    std::cout << "[duosight] MLX90640 CLI test\n";

    duosight::MLX90640Reader sensor("/dev/i2c-1", 0x33);

    if (!sensor.initialize()) {
        std::cerr << "Sensor init failed\n";
        return 1;
    }

    std::vector<float> frame;
    if (!sensor.readFrame(frame)) {
        std::cerr << "Frame read failed\n";
        return 1;
    }

    sensor.printSummary(frame);
    return 0;
}
