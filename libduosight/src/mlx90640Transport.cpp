#include "i2cUtils.hpp"
#include <cstdint>
#include <cstdio>

static duosight::I2cDevice* g_dev = nullptr;

extern "C" {

void mlx90640_set_i2c_device(duosight::I2cDevice* dev) {
    g_dev = dev;
}

int MLX90640_I2CRead(uint8_t addr, uint16_t reg, uint16_t len, uint16_t* out) {
    if (!g_dev || !g_dev->isOpen()) return -1;

    uint8_t tx[2] = { static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF) };
    uint8_t rx[2 * len] = {};

    if (!g_dev->writeThenRead(tx, 2, rx, sizeof(rx))) return -1;

    for (uint16_t i = 0; i < len; ++i) {
        out[i] = (rx[2 * i] << 8) | rx[2 * i + 1];
    }

    return 0;
}

int MLX90640_I2CWrite(uint8_t addr, uint16_t reg, uint16_t val) {
    if (!g_dev || !g_dev->isOpen()) return -1;

    uint8_t tx[4] = {
        static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF),
        static_cast<uint8_t>(val >> 8), static_cast<uint8_t>(val & 0xFF)
    };

    return g_dev->writeBytes(tx, 4) ? 0 : -1;
}

int MLX90640_I2CGeneralReset(void) {
    return 0;
}

} // extern "C"
