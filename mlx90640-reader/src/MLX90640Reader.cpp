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

bool MLX90640Reader::initialize()
{
    std::clog << "[MLX90640] --- initialize() ---\n";

    // 0) I2C sanity
    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    // 1) EEPROM -> params
    if (MLX90640_DumpEE(address_, eepromData_) != 0) {
        std::cerr << "[MLX90640] EEPROM dump failed\n";
        return false;
    }
    if (MLX90640_ExtractParameters(eepromData_, &params_) != 0) {
        std::cerr << "[MLX90640] Parameter extraction failed\n";
        return false;
    }
    std::clog << "[MLX90640] Parameters extracted OK\n";

    // 2) Configure via official helpers (avoid raw 0x800D writes)
    //    Start conservative; you can bump to FR8 later if you want snappier UI.
    {
        int rc = MLX90640_SetRefreshRate(address_, refresh::FR2);
        if (rc != 0) {
            std::cerr << "[MLX90640] SetRefreshRate(FR2) failed rc=" << rc << "\n";
            return false;
        }
        std::clog << "[MLX90640] Refresh rate set to FR2 (2 Hz full-frame)\n";
    }

    {
        int rc = MLX90640_SetChessMode(address_);
        if (rc != 0) {
            std::cerr << "[MLX90640] SetChessMode failed rc=" << rc << " (continuing)\n";
        } else {
            std::clog << "[MLX90640] Chess mode set\n";
        }
    }

    // Optional (if present in your API version)
    // {
    //     int rc = MLX90640_SetResolution(address_, 0x03);
    //     if (rc != 0) std::clog << "[MLX90640] SetResolution(3) not applied rc=" << rc << "\n";
    //     else         std::clog << "[MLX90640] Resolution set to 0x03\n";
    // }

    // 3) Read back CTRL (0x800D) for visibility only (no writes here)
    {
        uint16_t ctrl = 0;
        if (MLX90640_I2CRead(address_, 0x800D, 1, &ctrl) == 0) {
            int rr = (ctrl >> 7) & 0x07;                  // refresh code [9:7]
            static const float lut[8] = {0.5f,1,2,4,8,16,32,64};
            float hz = (rr >= 0 && rr < 8) ? lut[rr] : -1.0f;

            std::clog << "[MLX90640] CTRL(0x800D)=0x"
                      << std::hex << ctrl << std::dec
                      << "  refresh_code=" << rr
                      << " (" << hz << " Hz full-frame)\n";
        } else {
            std::cerr << "[MLX90640] Read CTRL (0x800D) failed (visibility only)\n";
        }
    }

    // 4) Clear NEW_DATA_READY (0x8000) once before first capture; log before/after
    {
        uint16_t statusBefore = 0xFFFF, statusAfter = 0xFFFF;
        (void)MLX90640_I2CRead(address_, 0x8000, 1, &statusBefore);

        int rc = MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        if (rc != 0) {
            std::cerr << "[MLX90640] Failed to clear status (0x8000) rc=" << rc << "\n";
            return false;
        }

        (void)MLX90640_I2CRead(address_, 0x8000, 1, &statusAfter);
        std::clog << "[MLX90640] Cleared NEW_DATA_READY  status: before=0x"
                  << std::hex << statusBefore << " after=0x" << statusAfter << std::dec << "\n";
    }

    // 5) Small settle delay after config (avoid racing the first read)
    usleep(5'000);

    std::clog << "[MLX90640] --- initialize() OK ---\n";
    return true;
}


bool MLX90640Reader::waitForNewFrame(int& subpageOut)
{
    for (int tries = 0; tries < Polling::MAX_RETRIES; ++tries)
    {
        uint16_t status = 0;
        if (!bus_->readRegister16(duosight::Status::REG, status)) {
            std::cerr << "[MLX90640] waitForNewFrame: I²C read failed for STATUS (0x8000)\n";
            return false;
        }

        std::clog << "[MLX90640] waitForNewFrame: STATUS=0x"
                  << std::hex << status << std::dec << "\n";

        // If NEW_DATA_READY bit set, decode subpage
        if (status & duosight::Status::NEW_DATA_READY) {
            const uint8_t lsb3 = static_cast<uint8_t>(status & 0x07);

            if (lsb3 == 0 || lsb3 == 1) {
                subpageOut = static_cast<int>(lsb3);

                if (status & duosight::Status::OVERRUN) {
                    std::cerr << "[MLX90640] waitForNewFrame: OVERRUN flag set in STATUS "
                              << "(data producer faster than consumer)\n";
                }

                std::clog << "[MLX90640] waitForNewFrame: New subpage "
                          << subpageOut << " ready.\n";
                return true; // fresh frame data
            }

            // Invalid/reserved subpage code
            std::cerr << "[MLX90640] waitForNewFrame: INVALID subpage code bits[2:0] = "
                      << static_cast<int>(lsb3)
                      << " (expected 0 or 1). Full STATUS=0x"
                      << std::hex << status << std::dec << "\n";
            return false;
        }

        // No new frame yet
        usleep(Polling::DELAY_US);
    }

    std::cerr << "[MLX90640] waitForNewFrame: Timeout waiting for NEW_DATA_READY\n";
    return false;
}



bool MLX90640Reader::readSubPage(int expectedSubpage, uint16_t* raw)
{
    // A) Latch STATUS before the RAM burst
    uint16_t status_first = 0;
    if (MLX90640_I2CRead(address_, 0x8000, 1, &status_first) != 0) {
        std::cerr << "[MLX90640] readSubPage: STATUS pre-read failed\n";
        return false;
    }

    const uint8_t sp = static_cast<uint8_t>(status_first & 0x07);
    if (sp != 0 && sp != 1) {
        std::cerr << "[MLX90640] readSubPage: INVALID subpage bits[2:0]="
                  << int(sp) << " STATUS=0x" << std::hex << status_first << std::dec << "\n";
        return false;
    }
    if ((expectedSubpage == 0 || expectedSubpage == 1) && sp != expectedSubpage) {
        std::cerr << "[MLX90640] readSubPage: pre-read STATUS says " << int(sp)
                  << " but expected " << expectedSubpage
                  << " (STATUS=0x" << std::hex << status_first << std::dec << ")\n";
        // Caller can decide to retry; treat as failure to keep semantics strict.
        return false;
    }

    // B) RAM burst (832 words)
    int rc = MLX90640_I2CRead(address_, 0x0400, 832, raw);
    if (rc != 0) {
        std::cerr << "[MLX90640] readSubPage: RAM read failed rc=" << rc << "\n";
        return false;
    }

    // C) Read CTRL1 once and append both trailing words using the pre-latched STATUS
    uint16_t ctrl1 = 0;
    if (MLX90640_I2CRead(address_, 0x800D, 1, &ctrl1) != 0) {
        std::cerr << "[MLX90640] readSubPage: CTRL1 read failed\n";
        return false;
    }

    raw[Geometry::PIXELS + 64] = static_cast<uint16_t>(status_first & 0x0001);  // index 832
    raw[Geometry::PIXELS + 65] = ctrl1;                                         // index 833

    if (status_first & duosight::Status::OVERRUN) {
        std::cerr << "[MLX90640] readSubPage: OVERRUN flagged\n";
    }

    // D) Clear STATUS so next measurement can proceed
    if (MLX90640_I2CWrite(address_, 0x8000, 0x0000) != 0) {
        std::cerr << "[MLX90640] readSubPage: Failed to clear STATUS (0x8000)\n";
        return false;
    }

    return true;
}

bool MLX90640Reader::combineChessTemperatures(const std::array<float, Geometry::PIXELS>& toA,
                              const std::array<float, Geometry::PIXELS>& toB,
                              int subpageA,   // 0 or 1
                              int subpageB,   // 0 or 1 (opposite of subpageA)
                              std::vector<float>& frameData)

{

 frameData.resize(Geometry::PIXELS);

    // Assign even/odd pixels to the subpage that produced them
    for (int idx = 0; idx < Geometry::PIXELS; ++idx) {
        const int row    = idx / Geometry::WIDTH;
        const int col    = idx % Geometry::WIDTH;
        const int parity = (row + col) & 1; // 0 = even, 1 = odd

        // If even pixels came from subpage 0, check if toA is subpage 0
        if (parity == 0) {
            frameData[idx] = (subpageA == 0) ? toA[idx] : toB[idx];
        } else {
            frameData[idx] = (subpageA == 1) ? toA[idx] : toB[idx];
        }
    }

    // --- Diagnostics ---
    auto mmA = std::minmax_element(toA.begin(), toA.end());
    auto mmB = std::minmax_element(toB.begin(), toB.end());
    std::clog << "[MLX90640] ToA[min,max]=[" << *mmA.first << "," << *mmA.second
              << "]  ToB[min,max]=[" << *mmB.first << "," << *mmB.second << "]\n";

    double meanA = 0.0, meanB = 0.0;
    for (int i = 0; i < Geometry::PIXELS; ++i) {
        const int r    = i / Geometry::WIDTH;
        const int c    = i % Geometry::WIDTH;
        const bool even = ((r + c) & 1) == 0;

        if (even == (subpageA == 0)) meanA += toA[i];
        else                         meanB += toB[i];
    }
    meanA /= (Geometry::PIXELS / 2);
    meanB /= (Geometry::PIXELS / 2);

    std::cout << std::fixed << std::setprecision(3)
              << "[MLX90640] To-mean  A=" << meanA << " °C"
              << "  B="                  << meanB << " °C"
              << "  Δ="                  << (meanA - meanB) << " °C\n";

return true;

}


bool MLX90640Reader::readFrame(std::vector<float>& frameData)
{
    using namespace Geometry;
    using namespace Polling;
    
    std::clog << "[MLX90640] --- Starting readFrame (helpers, two-subpages) ---\n";

    if (!bus_ || !bus_->isOpen()) {
        std::cerr << "[MLX90640] I²C device not open\n";
        return false;
    }

    std::array<uint16_t, Geometry::WORDS> fA{};
    std::array<uint16_t, Geometry::WORDS> fB{};

    // --- First subpage
    int seekFrame = 0;
    int subFrame = -1;
    bool got = false;

    for (int tries = 0; tries < Polling::MAX_RETRIES; ++tries) {
        if (waitForNewFrame(subFrame)) { got = true; break; }
        std::this_thread::sleep_for(std::chrono::microseconds(DELAY_US));
    }
    
    // just cannot a new frame and it timed out
    if (!got) {
        std::cerr << "[MLX90640] Timeout waiting for first NEW_DATA_READY\n";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }
    
    // duplicate frame scrap it
    if (subFrame!=seekFrame) {
        std::cerr << "[MLX90640] wrong subframe found: \n" << subFrame << " Skipping";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }

    std::clog << "[MLX90640] subpage ready: " << subFrame << " (reading RAM...)\n";

    // copy subpage over
    if (!readSubPage(subFrame, fA.data())) {
        std::cerr << "[MLX90640] readSubPage(first) failed\n";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }

    (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
    std::clog << "[MLX90640] First subpage captured and STATUS cleared\n";

    // --- Second subpage
    seekFrame = 1;
    got = false;  

    for (int tries = 0; tries < Polling::MAX_RETRIES; ++tries) {
        if (waitForNewFrame(subFrame)) { got = true; break; }
        std::this_thread::sleep_for(std::chrono::microseconds(DELAY_US));
    }
    
    // just cannot a new frame and it timed out
    if (!got) {
        std::cerr << "[MLX90640] Timeout waiting for first NEW_DATA_READY\n";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }
    
    // duplicate frame scrap it
    if (subFrame!=seekFrame) {
        std::cerr << "[MLX90640] wrong subframe found: \n" << subFrame << " Skipping";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }

    std::clog << "[MLX90640] subpage ready: " << subFrame << " (reading RAM...)\n";

    // copy subpage over
    if (!readSubPage(subFrame, fB.data())) {
        std::cerr << "[MLX90640] readSubPage(first) failed\n";
        (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
        return false;
    }

    (void)MLX90640_I2CWrite(address_, 0x8000, 0x0000);
    std::clog << "[MLX90640] Second subpage captured and STATUS cleared\n";
    std::clog << "[MLX90640] Calculating Ta/To per subpage...\n";

    const float eps = std::clamp(duosight::IRParams::EMISSIVITY, 0.1f, 1.0f);

    float TaA = MLX90640_GetTa(fA.data(), &params_);
    float TaB = MLX90640_GetTa(fB.data(), &params_);

    // Use average Ta if both valid, else the valid one
    float Ta = (std::isfinite(TaA) && std::isfinite(TaB)) ? 0.5f * (TaA + TaB)
             : (std::isfinite(TaA) ? TaA
             :  (std::isfinite(TaB) ? TaB : std::numeric_limits<float>::quiet_NaN()));

    std::clog << "[MLX90640]   TaA=" << TaA << "  TaB=" << TaB << "  Ta(avg)=" << Ta << " °C\n";
    if (!std::isfinite(Ta)) {
        std::cerr << "[MLX90640] Bad Ta from both subpages, skipping frame\n";
        return false;
    }

    std::array<float, Geometry::PIXELS> toA{}, toB{};
        
    MLX90640_CalculateTo(fA.data(), &params_, eps, Ta, toA.data());
    MLX90640_CalculateTo(fB.data(), &params_, eps, Ta, toB.data());

    if (!combineChessTemperatures(toA, toB, 0, 1, frameData)) 
    {
        std::cerr << "[MLX90640] Calculation error\n";


    }

    return true;
}

} // namespace duosight
