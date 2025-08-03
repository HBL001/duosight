/**
 * @file MLX90640Reader.h
 * @brief High-level interface for MLX90640 thermal sensor.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Wraps the Melexis MLX90640 API and I2C interface to support
 *   initialization and frame capture of thermal image data.
 */

#pragma once

#include "i2cUtils.h"
#include <string>
#include <vector>
#include <memory>

namespace duosight {

class MLX90640Reader {
public:
    MLX90640Reader(const std::string& i2cPath = "/dev/i2c-1", uint8_t address = 0x33);
    ~MLX90640Reader();

    bool initialize();
    bool readFrame(std::vector<float>& frameData); // 768 floats (32x24)

    void printSummary(const std::vector<float>& frameData);

private:
    std::unique_ptr<I2cDevice> i2c_;
    uint8_t address_;

    // Raw EEPROM & calibration data
    int eepromData_[832];
    float params_[MLX90640_PARAMS_SIZE];
};

} // namespace duosight
