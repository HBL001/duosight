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
inline constexpr uint16_t SUBPAGE_MASK    = 0b0000'0000'0000'0111; // bits 0-2
inline constexpr uint16_t SUBPAGE_BIT0    = 0b0000'0000'0000'0001; // bit 0
inline constexpr uint16_t NEW_DATA_READY  = 0b0000'0000'0000'1000; // bit 3
inline constexpr uint16_t OVERRUN         = 0b0000'0000'0001'0000; // bit 4
inline constexpr uint16_t MEAS_START      = 0b0000'0000'0010'0000; // bit 5
inline constexpr uint16_t EE_BUSY         = 0b0000'0000'0100'0000; // bit 6
inline constexpr uint16_t FT_BUSY         = 0b0000'0000'1000'0000; // bit 7
inline constexpr uint16_t DIAG0           = 0b0000'0001'0000'0000; // bit 8
inline constexpr uint16_t DIAG1           = 0b0000'0010'0000'0000; // bit 9
inline constexpr uint16_t EN_OVERWRITE    = 0b0000'0100'0000'0000; // bit 10
inline constexpr uint16_t DATA_HOLD       = 0b0000'1000'0000'0000; // bit 11
inline constexpr uint16_t DEVICE_BUSY     = 0b0001'0000'0000'0000; // bit 12
inline constexpr uint16_t RSVD13          = 0b0010'0000'0000'0000; // bit 13
inline constexpr uint16_t RSVD14          = 0b0100'0000'0000'0000; // bit 14
inline constexpr uint16_t INTERFACE_ERROR = 0b1000'0000'0000'0000; // bit 15
} // namespace Status

namespace refresh
{
    // Mask already shifted to bits 7-5
    inline constexpr uint16_t MASK   = 0b1110'0000;      // bits 7-5

    // 3-bit codes ( *unshifted* – pass straight to MLX90640_SetRefreshRate )
    inline constexpr uint8_t  FR0P5  = 0b000;            // 0.5 Hz
    inline constexpr uint8_t  FR1    = 0b001;            // 1 Hz
    inline constexpr uint8_t  FR2    = 0b010;            // 2 Hz
    inline constexpr uint8_t  FR4    = 0b011;            // 4 Hz
    inline constexpr uint8_t  FR8    = 0b100;            // 8 Hz
    inline constexpr uint8_t  FR16   = 0b101;            // 16 Hz
    inline constexpr uint8_t  FR32   = 0b110;            // 32 Hz
    inline constexpr uint8_t  FR64   = 0b111;            // 64 Hz
} // namespace refresh




} // namespace duosight
