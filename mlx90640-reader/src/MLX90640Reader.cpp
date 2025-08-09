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
#include <algorithm>

namespace duosight {

MLX90640Reader::MLX90640Reader(I2cDevice& bus, uint8_t address)
    : bus_{&bus}, address_{address}
{
    mlx90640_set_i2c_device(bus_);        
}

MLX90640Reader::~MLX90640Reader() {}


refresh::RefreshInfo MLX90640Reader::readRefreshRate(bool verbose) const
{
    uint16_t ctrl = 0;
    refresh::RefreshInfo info{ -1, -1.0f, -1.0f };
    if (MLX90640_I2CRead(address_, 0x800D, 1, &ctrl) != 0) {
        if (verbose) {
            std::cerr << "[MLX90640] readRefreshRate: failed to read CTRL1 (0x800D)\n";
        }
        return info;
    }

    static const float lut[8] = { 0.5f, 1, 2, 4, 8, 16, 32, 64 };

    info.code = (ctrl >> 7) & 0x07;
    if (info.code >= 0 && info.code < 8) {
        info.hz = lut[info.code];
        info.subpage_period_s = 1.0f / (info.hz * 2.0f);
    }

    if (verbose) {
        std::clog << "[MLX90640] CTRL(0x800D)=0x"
                  << std::hex << ctrl << std::dec
                  << "  refresh_code=" << info.code
                  << " (" << info.hz << " Hz full-frame, "
                  << info.subpage_period_s * 1000.0f << " ms/subpage)\n";
    }

    return info;
}


bool MLX90640Reader::initialize()
{
    std::clog << "[MLX90640] --- initialize() ---\n";

    // ─────────────────────────────────────────────
    // 0) I²C sanity
    // ─────────────────────────────────────────────
    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] ❌ I²C device not open\n";
        return false;
    }

    // ─────────────────────────────────────────────
    // 1) EEPROM → params
    // ─────────────────────────────────────────────
    if (MLX90640_DumpEE(address_, eepromData_) != 0) {
        std::cerr << "[MLX90640] EEPROM dump failed\n";
        return false;
    }
    if (MLX90640_ExtractParameters(eepromData_, &params_) != 0) {
        std::cerr << "[MLX90640] Parameter extraction failed\n";
        return false;
    }
    std::clog << "[MLX90640] Parameters extracted OK\n";

    // ─────────────────────────────────────────────
    // 2) Configure sensor
    // ─────────────────────────────────────────────
    if (int rc = MLX90640_SetRefreshRate(address_, refresh::FR2); rc != 0) {
        std::cerr << "[MLX90640] SetRefreshRate(FR2) failed rc=" << rc << "\n";
        return false;
    }
    std::clog << "[MLX90640] Refresh rate set to FR2 (2 Hz full-frame)\n";

    if (int rc = MLX90640_SetChessMode(address_); rc != 0) {
        std::cerr << "[MLX90640] SetChessMode failed rc=" << rc << " (continuing)\n";
    } else {
        std::clog << "[MLX90640] Chess mode set\n";
    }

    // ─────────────────────────────────────────────
    // 3) TIMING SETUP — CRITICAL for frame capture
    //    This determines our subpage period for polling
    // ─────────────────────────────────────────────
    auto ri = readRefreshRate(true); // logs CTRL1 & derived Hz/ms
    if (ri.code != refresh::FR2) {
        std::cerr << "[MLX90640] ⚠ Refresh rate readback (" << ri.code
                  << ") does not match requested FR2\n";
    }
    // NOTE: save ri.subpage_period_s somewhere if you want to use it globally

    // ─────────────────────────────────────────────
    // 4) Clear NEW_DATA_READY before first capture
    // ─────────────────────────────────────────────
    uint16_t statusBefore = 0, statusAfter = 0;
    if (MLX90640_I2CRead(address_, 0x8000, 1, &statusBefore) != 0)
        std::cerr << "[MLX90640] ⚠ Failed to read STATUS before clear\n";

    if (int rc = MLX90640_I2CWrite(address_, 0x8000, 0x0000); rc != 0) {
        std::cerr << "[MLX90640] Failed to clear status (0x8000) rc=" << rc << "\n";
        return false;
    }

    if (MLX90640_I2CRead(address_, 0x8000, 1, &statusAfter) != 0)
        std::cerr << "[MLX90640] ⚠ Failed to read STATUS after clear\n";

    std::clog << "[MLX90640] Cleared NEW_DATA_READY status: before=0x"
              << std::hex << statusBefore << " after=0x" << statusAfter << std::dec << "\n";

    // ─────────────────────────────────────────────
    // 5) Settle delay
    // ─────────────────────────────────────────────
    usleep(5'000);

    std::clog << "[MLX90640] --- initialize() OK ---\n";
    return true;
}


void MLX90640Reader::sleepNow(int delay)
{
    std::this_thread::sleep_for(std::chrono::microseconds(delay));
}


bool MLX90640Reader::readFrame(std::vector<float> &frameData)
{
    using namespace duosight;

    float Ta;
    
    
    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    // --- Read refresh rate info ---
    const auto ri = readRefreshRate(true); // verbose prints rate/subpage time
    if (ri.code < 0 || ri.subpage_period_s <= 0.0f) {
        std::cerr << "[MLX90640] ❌ Invalid refresh rate — cannot proceed\n";
        return false;
    }

    const int us_per_poll   = Polling::DELAY_US;
    const int max_polls_per = static_cast<int>((ri.subpage_period_s * 1.25f * 1'000'000) / us_per_poll);
    
    std::clog << "[MLX90640]   Max polls per subpage: " << max_polls_per << "\n";

    std::array<uint16_t, Geometry::WORDS> subpage0{};
    std::array<uint16_t, Geometry::WORDS> subpage1{};
    std::vector<float> subframe0{};
    std::vector<float> subframe1{};
    
    int sp = -1;

    // --- First subpage ---
    sp = MLX90640_GetFrameData(address_, subpage0.data());
    if (sp != 0) {
        std::clog << "[MLX90640] GetFrameData failed for first subpage rc=" << sp << "\n";
        return false;
    }

    std::clog << "\n";

    Ta = MLX90640_GetTa(subpage0.data(), &params_);
    std::clog << "[MLX90640] --- Dump first 40 words of subpage0 \n";
    
    for (int i = 0; i < 40; ++i) {
        std::clog << std::setw(2) << i << ": " << std::hex << std::showbase
                  << subpage0[i] << std::dec << " ";
    }
    
    std::clog << "\n";
    
    // --- Convert to temperatures ---
    subframe0.resize(Geometry::WIDTH * Geometry::HEIGHT);
        
    MLX90640_CalculateTo(subpage0.data(),
                         &params_,
                         IRParams::EMISSIVITY,
                         Ta,
                         subframe0.data());

    std::clog << "[MLX90640] --- Dump first 40 words of To for subpage0 \n";

        for (int i = 0; i < 40; ++i) {
        std::clog << std::setw(2) << i << ": " << std::hex << std::showbase
                  << subframe0[i] << std::dec << " ";
        }
    
    std::clog << "\n";
        
    // --- Second subpage ---
    sp = MLX90640_GetFrameData(address_, subpage1.data());
    if (sp != 1) {
        std::clog << "[MLX90640] GetFrameData failed for second subpage rc=" << sp << "\n";
        return false;
    } 
       
    Ta = MLX90640_GetTa(subpage1.data(), &params_);
    std::clog << "[MLX90640] --- Dump first 40 words of subpage1 \n";
    
    for (int i = 0; i < 40; ++i) {
        std::clog << std::setw(2) << i << ": " << std::hex << std::showbase
                  << subpage1[i] << std::dec << " ";
    }

    std::clog << "\n";

    // --- Convert to temperatures ---
    subframe1.resize(Geometry::WIDTH * Geometry::HEIGHT);

    MLX90640_CalculateTo(subpage1.data(),
                         &params_,
                         IRParams::EMISSIVITY,
                         Ta,
                         subframe1.data());

    
    std::clog << "[MLX90640] --- Dump first 40 words of To for subpage0 \n";

        for (int i = 0; i < 40; ++i) {
        std::clog << std::setw(2) << i << ": " << std::hex << std::showbase
                  << subframe1[i] << std::dec << " ";
        }
  
    std::clog << "\n";

    
    subframe1.resize(Geometry::WIDTH * Geometry::HEIGHT);


    // --- Merge subpages into a complete frame ---
    for (int i = 0; i < Geometry::PIXELS; ++i) {
        frameData[i] = (Geometry::PIXEL_TO_SUBPAGE[i] == 0)
                    ? subframe0[i]
                    : subframe1[i];

        if (i<41) {
           std::clog << std::setw(2) << i << ": " << std::hex << std::showbase
                  << frameData[i] << std::dec << " ";
        }
    }

    return true;
}



} // namespace duosight
