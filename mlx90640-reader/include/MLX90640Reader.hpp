// ────────────────────────────────────────────────────────────────
// MLX90640Reader.hpp
// High-level wrapper around the Melexis MLX90640 API that re-uses an
// already-opened I²C bus handle (duosight::I2cDevice) rather than
// opening /dev/i2c-X a second time.
//
// © 2025 Highland Biosciences — Dr Richard Day — SPDX-License-Identifier: MIT
// ────────────────────────────────────────────────────────────────
#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <array>
#include "MLX90640Regs.hpp"
#include "MLX90640_API.h"
#include "i2cUtils.hpp"

#ifndef MLX90640_PARAMS_SIZE
#define MLX90640_PARAMS_SIZE 1664
#endif

namespace duosight
{

// -----------------------------------------------------------------
// Refresh-rate constants and table
// -----------------------------------------------------------------
namespace refresh
{
    inline constexpr unsigned SHIFT = 5;              // bits 7:5 in CTRL1
    inline constexpr uint16_t MASK  = 0b111 << SHIFT; // mask for bits 7:5

    // 3-bit refresh codes (unshifted)
    inline constexpr uint8_t FR0P5 = 0b000;  // 0.5 Hz full frame
    inline constexpr uint8_t FR1   = 0b001;  // 1 Hz
    inline constexpr uint8_t FR2   = 0b010;  // 2 Hz
    inline constexpr uint8_t FR4   = 0b011;  // 4 Hz
    inline constexpr uint8_t FR8   = 0b100;  // 8 Hz
    inline constexpr uint8_t FR16  = 0b101;  // 16 Hz
    inline constexpr uint8_t FR32  = 0b110;  // 32 Hz
    inline constexpr uint8_t FR64  = 0b111;  // 64 Hz

    struct RateInfo {
        float hz_full_frame;    // Full-frame rate in Hz
        float sec_subpage;      // Seconds per subpage in chess mode
    };

    inline constexpr RateInfo TABLE[] = {
        {0.5f,    1.0f},         // FR0P5
        {1.0f,    0.5f},         // FR1
        {2.0f,    0.25f},        // FR2
        {4.0f,    0.125f},       // FR4
        {8.0f,    0.0625f},      // FR8
        {16.0f,   0.03125f},     // FR16
        {32.0f,   0.015625f},    // FR32
        {64.0f,   0.0078125f}    // FR64
    };

    struct RefreshInfo {
        int   code;              // 0..7 (refresh code from register)
        float hz;                // full-frame rate in Hz (-1 if invalid)
        float subpage_period_s;  // seconds per subpage (-1 if invalid)
    };
} // namespace refresh


// -----------------------------------------------------------------
// MLX90640Reader class
// -----------------------------------------------------------------
class MLX90640Reader {
public:
    MLX90640Reader(I2cDevice &bus, uint8_t address);
    ~MLX90640Reader();

    bool initialize();                                 
    bool waitForNewFrame(int &subpageOut, std::array<uint16_t, duosight::Geometry::WORDS> &dest);
    void sleepNow(int delay);
    bool ackClear(void);
    bool readFrame(std::vector<float> &frame);
    void dumpSubpage(const char *label, const std::array<uint16_t, Geometry::WORDS> &buf);
    bool readSubPage(int subpage, uint16_t *raw);
    
    int  MLX90640_GrabSubPage(uint8_t sa, uint16_t *frame);
    int  grabSubPage(uint16_t *raw);                          ///< burst + clear

    void printSummary(const std::vector<float>& frame) const; ///< debug

    // New helper for CTRL1-based timing
    refresh::RefreshInfo readRefreshRate(bool verbose = false) const;

private:
    /* The reader does *not* own the bus; caller keeps it alive. */
    I2cDevice* bus_ {nullptr};
    uint8_t    address_ {0x33};

    // Calibration and scratch buffers
    uint16_t       eepromData_[832] {};
    paramsMLX90640 params_{};
};

} // namespace duosight
