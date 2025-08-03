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
#include "i2cUtils.h"
#include "mlx90640Transport.h"

#include <vector>
#include <iostream>

int main() {
    std::cout << "[duosight] MLX90640 CLI test" << std::endl;

    // Set up I2C device
    duosight::I2cDevice i2c("/dev/i2c-3", 0x33);  // Adjust if your bus/address differs
    if (!i2c.isOpen()) {
        std::cerr << "❌ I2C open failed (/dev/i2c-1 @ 0x33)" << std::endl;
        return 1;
    }

    // Link the I2C device to Melexis API transport layer
    mlx90640_set_i2c_device(&i2c);

    // Sensor abstraction
    duosight::MLX90640Reader sensor("/dev/i2c-3", 0x33);


    if (!sensor.initialize()) {
        std::cerr << "❌ Sensor init failed" << std::endl;
        return 1;
    }

    std::vector<float> frame;
    if (!sensor.readFrame(frame)) {
        std::cerr << "❌ Frame read failed" << std::endl;
        return 1;
    }

    sensor.printSummary(frame);
    return 0;
}
