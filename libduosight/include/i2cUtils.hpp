/**
 * @file i2cUtils.h
 * @brief I2C communication utility class for embedded Linux.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Provides a lightweight, object-oriented wrapper around /dev/i2c-X
 *   for use in sensor applications such as MLX90640.
 */

#pragma once

#include <cstdint>
#include <string>

namespace duosight {

class I2cDevice {
public:
    I2cDevice(const std::string& devicePath, uint8_t address);
    ~I2cDevice();

    bool isOpen() const;

    bool writeBytes(const uint8_t* data, size_t length);
    bool readBytes(uint8_t* buffer, size_t length);
    bool writeThenRead(const uint8_t* txData, size_t txLen, uint8_t* rxData, size_t rxLen);
    bool readRegister16(uint16_t reg, uint16_t& value);
    bool writeRegister16(uint16_t reg, uint16_t value);
    
private:
    int fd_;
    uint8_t addr_;
};

} // namespace duosight
