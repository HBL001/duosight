/**
 * @file MLX90640Reader.cpp
 * @brief Implementation of MLX90640Reader using Melexis C API and duosight::I2cDevice.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Provides initialization of the MLX90640 sensor and reading of thermal frame data.
 *   Uses the official Melexis API for parameter extraction and temperature conversion.
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <chrono>

#include "i2cUtils.hpp"
#include "MLX90640Reader.hpp"
#include "MLX90640_API.h"

namespace duosight {

MLX90640Reader::MLX90640Reader(const std::string& i2cPath, uint8_t address)
    : address_(address)
{
    i2c_ = std::make_unique<I2cDevice>(i2cPath, address);
}

MLX90640Reader::~MLX90640Reader() {}


bool MLX90640Reader::initialize() {
    if (!i2c_ || !i2c_->isOpen()) {
        std::cerr << "[MLX90640] I2C device not open\n";
        return false;
    }

    std::clog << "[MLX90640] Dumping EEPROM...\n";
    if (MLX90640_DumpEE(address_, eepromData_) != 0) {
        std::cerr << "[MLX90640] Failed to dump EEPROM\n";
        return false;
    }

    std::clog << "[MLX90640] Extracting calibration parameters...\n";
    if (MLX90640_ExtractParameters(eepromData_, params_) != 0) {
        std::cerr << "[MLX90640] Failed to extract sensor parameters\n";
        return false;
    }

    // Set refresh rate (0x00 to 0x07)
    constexpr int refreshRateCode = 0x05; // 16 Hz
    if (MLX90640_SetRefreshRate(address_, refreshRateCode) != 0) {
        std::cerr << "[MLX90640] Failed to set refresh rate to code " << refreshRateCode << "\n";
        return false;
    } else {
        std::clog << "[MLX90640] Set refresh rate to " << (1 << (refreshRateCode - 1)) << " Hz\n";
    }

    if (MLX90640_SetChessMode(address_) != 0) {
        std::cerr << "[MLX90640] Failed to set Chess Mode\n";
        return false;
    } else {
        std::clog << "[MLX90640] Set Chess Mode\n";
    }

    return true;
}


bool MLX90640Reader::waitForNewFrame(I2cDevice& i2c, int& subpageOut, int maxRetries, int delayUs) {
    
    static int lastSubpage = -1;
    
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        uint16_t status = 0;
    
        if (!i2c.readRegister16(STATUS_REGISTER, status)) {
            std::cerr << "[MLX90640] Failed to read status register\n";
            return false;
        }

        if (status & NEW_DATA_MASK) {
            subpageOut = status & SUB_FRAME_MASK; // Bit 0 = subframe number
            return true;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(delayUs));
    }

    std::cerr << "[MLX90640] Timed out waiting for new data (bit 3)\n";
    return false;
}

bool MLX90640Reader::readFrame(std::vector<float>& frameData) {
    static int lastSubpage = -1;
    static std::vector<float> subframe0(HEIGHT * WIDTH, 0.0f);
    static std::vector<float> subframe1(HEIGHT * WIDTH, 0.0f);

    if (!i2c_ || !i2c_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    uint16_t raw[WIDTH * HEIGHT + 66]; // 834 words
    int subpage = -1;

    // 1) Wait for new data (bit 3) and capture which subpage (bit 1)
    if (!waitForNewFrame(*i2c_, subpage, MAXRETRIES, DELAYUS)) {
        std::cerr << "[MLX90640] Timed out waiting for new frame\n";
        return false;
    }

    // 2) If it's the same subpage we already handled, bail out quickly
    if (subpage == lastSubpage) {
        std::clog << "[MLX90640] Subpage " << subpage << " already processed, skipping\n";
        std::this_thread::sleep_for(std::chrono::microseconds(DELAYUS));
        return false;
    }

    std::clog << "[MLX90640] New subpage available: " << subpage << "\n";

    // 3) Grab that subframe’s raw data
    if (MLX90640_GetFrameData(address_, raw) != 0) {
        std::cerr << "[MLX90640] Failed to read raw frame data\n";
        return false;
    }

    // 4) Convert the raw subframe into temperatures
    auto &dst = (subpage == 0) ? subframe0 : subframe1;
    MLX90640_CalculateTo(raw, params_, emissivity_, ambientTemp_, dst.data());
    
    lastSubpage = subpage;

    // 5) Clear that “new data” bit (bit 3) by writing a 1 to it
    if (!i2c_->writeRegister16(STATUS_REGISTER, NEW_DATA_MASK)) {
        std::cerr << "[MLX90640] Failed to clear new-data flag\n";
    } else {
        std::clog << "[MLX90640] Cleared new-data flag for subpage " << subpage << "\n";
    }

    // 6) If we now have both halves, merge them and return true
    if (subframe0[0] != 0.0f && subframe1[0] != 0.0f) {
        frameData.resize(HEIGHT * WIDTH);
        for (size_t i = 0; i < frameData.size(); ++i) {
            // average the two interleaved chess-pattern subframes
            frameData[i] = 0.5f * (subframe0[i] + subframe1[i]);
        }
        return true;
    }

    // Not yet: still waiting for the other subframe
    return false;
}





void MLX90640Reader::printSummary(const std::vector<float>& frameData) {
    if (frameData.size() != 768) {
        std::cerr << "[MLX90640] Invalid frame size\n";
        return;
    }

    float minT = frameData[0];
    float maxT = frameData[0];
    float sumT = 0.0f;

    for (float t : frameData) {
        if (t < minT) minT = t;
        if (t > maxT) maxT = t;
        sumT += t;
    }

    float avgT = sumT / frameData.size();

    std::cout << "[MLX90640] Frame: min=" << minT
              << "°C max=" << maxT
              << "°C avg=" << avgT << "°C\n";
}

} // namespace duosight
