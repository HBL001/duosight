// ────────────────────────────────────────────────────────────────
// MLX90640Reader.hpp
// High‑level wrapper around the Melexis MLX90640 API that re‑uses an
// already‑opened I²C bus handle (duosight::I2cDevice) rather than
// opening /dev/i2c‑X a second time.
//
// © 2025 Highland Biosciences — Dr Richard Day — SPDX‑License‑Identifier: MIT
// ────────────────────────────────────────────────────────────────
#pragma once

#include <vector>
#include "MLX90640Regs.hpp"
#include "MLX90640_API.h"
#include "i2cUtils.hpp"

#ifndef MLX90640_PARAMS_SIZE
#define MLX90640_PARAMS_SIZE 1664
#endif

namespace duosight {

class MLX90640Reader {
public:
    MLX90640Reader(I2cDevice &bus, uint8_t address);
    ~MLX90640Reader();

    bool initialize();                                        ///< EEPROM → params
    bool readFrame(std::vector<float>& frame);                ///< full 32×24

    // --- low‑level helpers --------------------------------------------
    bool waitForNewFrame(int& subpageOut);                    ///< non‑blocking
    bool readSubPage(int subpage, uint16_t *raw);
    int MLX90640_GrabSubPage(uint8_t sa, uint16_t *frame);
    int grabSubPage(uint16_t *raw); ///< burst + clear

    void printSummary(const std::vector<float>& frame) const; ///< debug

private:
    /* The reader does *not* own the bus; caller keeps it alive. */
    I2cDevice* bus_ {nullptr};
    uint8_t    address_ {0x33};

    // Calibration and scratch buffers
    uint16_t       eepromData_[832] {};
    paramsMLX90640 params_[MLX90640_PARAMS_SIZE] {};
    
    void printSummary(const std::vector<float> &frameData);
};

} // namespace duosight
