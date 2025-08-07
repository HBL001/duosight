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
#include <array>
#include <cmath>
#include <iomanip> 

#include "i2cUtils.hpp"
#include "MLX90640Reader.hpp"
#include "MLX90640Regs.hpp"
#include "MLX90640_API.h"
#include "mlx90640Transport.h"

namespace duosight {

MLX90640Reader::MLX90640Reader(I2cDevice& bus, uint8_t address)
    : bus_{&bus}, address_{address}
{
    mlx90640_set_i2c_device(bus_);        
}

MLX90640Reader::~MLX90640Reader() {}

bool MLX90640Reader::initialize()
{
    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    // 1) Read and parse EEPROM → calibration parameters
    if (MLX90640_DumpEE(address_, eepromData_) != 0) {
        std::cerr << "[MLX90640] EEPROM dump failed\n";
        return false;
    }
    if (MLX90640_ExtractParameters(eepromData_, params_) != 0) {
        std::cerr << "[MLX90640] Parameter extraction failed\n";
        return false;
    }

    // 2) Set desired refresh rate (2 Hz full-frame → 4 Hz sub-pages)
    if (MLX90640_SetRefreshRate(address_, refresh::FR2) != 0) {
        std::cerr << "[MLX90640] Refresh-rate set failed\n";
        return false;
    }

    // 3) Enable chess-pattern (sub-page) mode in Control Register 1 (0x800D)
    uint16_t ctrl;
    if (MLX90640_I2CRead(address_, 0x800D, 1, &ctrl) != 0) {
        std::cerr << "[MLX90640] Read CTRL (0x800D) failed\n";
        return false;
    }
    //   • bit 0 = 1 → sub-page mode
    //   • bit 3 = 0 → auto-repeat off
    ctrl = (ctrl & ~(1u << 3))   // clear bit-3
         |  (1u << 0);           // set bit-0
    if (MLX90640_I2CWrite(address_, 0x800D, ctrl) != 0) {
        std::cerr << "[MLX90640] Write CTRL (0x800D) failed\n";
        return false;
    }

    return true;
}


bool MLX90640Reader::waitForNewFrame(int& subpageOut)
{
    uint16_t status;
    if (!bus_->readRegister16(duosight::Status::REG, status))
        return false;                                 // I²C error

    if (!(status & duosight::Status::NEW_DATA_READY))
        return false;                                 // nothing new yet

    subpageOut = status & duosight::Status::SUBPAGE_BIT0; // 0 or 1
    
    return true;                                      // ✔ fresh sub-page
}

bool MLX90640Reader::readSubPage(int subpage, uint16_t* raw) {
    // 1) Burst-read RAM 0x0400–0x06FF (832 words) into raw[]
    if (MLX90640_I2CRead(address_, 0x0400, 832, raw) != 0) {
        return false;
    }

    // 2) Clear NEW_DATA_READY (bit-3) and OVERRUN (bit-4) in the STATUS register
    //    so the sensor can start the next measurement cycle cleanly.
    const uint16_t clearMask = Status::NEW_DATA_READY | Status::OVERRUN;
    if (!bus_->writeRegister16(Status::REG, clearMask)) {
        std::cerr << "[MLX90640] Failed to clear status bits\n";
        return false;
    }

    return true;
}


bool MLX90640Reader::readFrame(std::vector<float>& frameData) {
    // 1) Sanity-check I²C bus
    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    // 2) Let Melexis API do the chess-pattern interleave for us
    //    (returns 832 words: 768 pixels + 64 overhead)
    constexpr int MAX_TRIES = 150;  // ~750 ms @ 2 Hz
    uint16_t burst[Geometry::WIDTH * Geometry::HEIGHT + 66];
    int status = -1;
    for (int i = 0; i < MAX_TRIES; ++i) {
        status = MLX90640_GetFrameData(address_, burst);
        if (status == 0) break;  // both sub-pages read & merged
        std::this_thread::sleep_for(
            std::chrono::microseconds(Polling::DELAY_US)
        );
    }
    if (status != 0) {
        std::cerr << "[MLX90640] GetFrameData timed out (" << status << ")\n";
        return false;
    }

    // Convert the 768 raw codes → °C
    constexpr size_t W = Geometry::WIDTH, H = Geometry::HEIGHT, N = W*H;
    frameData.resize(N);
    
    // *** No filtering before this point ***
    float Ta = MLX90640_GetTa(burst, params_);
    if (std::isnan(Ta) || std::isinf(Ta)) {
        std::cerr << "Bad Ta, skipping frame\n";
        return false;
    }

    double sum0 = 0, sum1 = 0;
    for (int idx = 0; idx < 768; ++idx) {
        bool inSub0 = ((idx / 32 + idx % 32) & 1) == 0;
        if (inSub0) sum0 += burst[idx];
        else        sum1 += burst[idx];
        }
std::cout << "Raw mean sub0=" << sum0 / 384.0
          << "  sub1=" << sum1 / 384.0 << '\n';


    MLX90640_CalculateTo(
        burst,             // const uint16_t* raw data[0..767]
        params_,           // const paramsMLX90640* calibration data
        IRParams::EMISSIVITY,
        Ta,                // float ambient temperature
        frameData.data()   // float* output array
    );


// -----------------------------------------------------------------------------

sum0 = 0.0;
sum1 = 0.0;

for (std::size_t idx = 0; idx < 768; ++idx) {
    bool inSub0 = ((idx / W + idx % W) & 1) == 0;  // chess test
    if (inSub0) sum0 += frameData[idx];
    else        sum1 += frameData[idx];
}

double mean0 = sum0 / 384.0;
double mean1 = sum1 / 384.0;

std::cout << std::fixed << std::setprecision(3)
          << "To-mean sub0 = " << mean0
          << " °C   sub1 = " << mean1
          << " °C   Δ = "   << (mean0 - mean1) << " °C\n";

    return true;
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
