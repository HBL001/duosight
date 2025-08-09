/**
 * @file    MLX90640Regs.hpp
 * @brief   Compile-time constants and bit-masks for the Melexis MLX90640 32×24
 *          infrared array.  All symbols are `constexpr` so the header can be
 *          included by any translation unit without violating the One-Definition
 *          Rule.  Nothing here incurs any run-time or linkage cost.
 *
 *  Sections
 *  ─────────
 *   • Geometry                             – sensor pixel matrix
 *   • Polling parameters                   – helper values for wait-loops
 *   • Default IR scene assumptions         – emissivity & ambient temperature
 *   • ADC resolution codes (reg 0x800D)    – 16- to 19-bit
 *   • Status register 0x8000 bit masks     – every documented flag
 *
 *  Copyright (c) 2025  Highland Biosciences  –  Dr Richard Day
 *  SPDX-License-Identifier: MIT
 * 
 */

#pragma once
#include <cstdint>
#include <array>

namespace duosight
{

namespace Bus {
inline constexpr const char* DEV        = "/dev/i2c-3";
inline constexpr uint8_t     SLAVE_ADDR = 0x33;
}

// ────────────────────────────────────────────────────────────────
//  Geometry
// ────────────────────────────────────────────────────────────────
namespace Geometry {
inline constexpr std::size_t WIDTH  = 32;
inline constexpr std::size_t HEIGHT = 24;
inline constexpr int PIXELS = WIDTH * HEIGHT;   // 768
inline constexpr int TAIL   = 64;               // NOT 64
inline constexpr int WORDS  = PIXELS + TAIL + 2;    // 834

inline constexpr std::array<uint8_t, PIXELS> PIXEL_TO_SUBPAGE = []() {
    std::array<uint8_t, PIXELS> lut{};
    for (int row = 0; row < static_cast<int>(HEIGHT); ++row) {
        for (int col = 0; col < static_cast<int>(WIDTH); ++col) {
            int idx = row * static_cast<int>(WIDTH) + col;
            lut[idx] = static_cast<uint8_t>((row + col) & 1); // 0 or 1
        }
    }
    return lut;
}();

} // namespace Geometry

// ────────────────────────────────────────────────────────────────
//  Poll-loop parameters
// ────────────────────────────────────────────────────────────────
namespace Polling {
inline constexpr int MAX_RETRIES = 150;
inline constexpr int DELAY_US    = 5'000;   // 5 ms
} // namespace Polling

// ────────────────────────────────────────────────────────────────
//  Default environment assumptions
// ────────────────────────────────────────────────────────────────
namespace IRParams {
inline constexpr float EMISSIVITY   = 0.95f; // generic matte surface
inline constexpr float AMBIENT_TEMP = 25.0f; // °C
} // namespace IRParams

// ────────────────────────────────────────────────────────────────
//  ADC resolution selector (bits [4:3] of CTRL reg 0x800D)
// ────────────────────────────────────────────────────────────────
namespace Resolution {
inline constexpr int ADC_16bit = 0; // fastest  – highest noise
inline constexpr int ADC_17bit = 1;
inline constexpr int ADC_18bit = 2; // power-on default
inline constexpr int ADC_19bit = 3; // slowest – lowest noise
} // namespace Resolution

// ────────────────────────────────────────────────────────────────
//  STATUS register (0x8000) – full bit-map
// ────────────────────────────────────────────────────────────────
namespace Status {
inline constexpr uint16_t REG = 0x8000;      // register address

// Individual bit masks
inline constexpr uint16_t SUBPAGE_MASK    = 0b0000'0000'0000'0001; // bit 0: which subpage
inline constexpr uint16_t NEW_DATA_READY  = 0b0000'0000'0000'1000; // bit 3: New data available in RAM
inline constexpr uint16_t OVERRUN         = 0b0000'0000'0001'0000; // bit 4: Data overwritten before read
inline constexpr uint16_t INTERFACE_ERROR = 0b1000'0000'0000'0000; // bit 15: Interface error
} // namespace Status

} // namespace duosight
