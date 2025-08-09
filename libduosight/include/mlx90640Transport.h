/**
 * @file mlx90640Transport.h
 * @brief Transport layer glue for Melexis MLX90640 API using I2cDevice.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Allows MLX90640 API to call into C++ I2cDevice via global context.
 *   Required to interface the Melexis API with Linux ioctl-based I2C access.
 */

#pragma once

#include "i2cUtils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void mlx90640_set_i2c_device(duosight::I2cDevice* dev);

int MLX90640_I2CRead(uint8_t addr, uint16_t reg, uint16_t len, uint16_t* out);
int MLX90640_I2CWrite(uint8_t addr, uint16_t reg, uint16_t val);
int MLX90640_I2CGeneralReset(void);

#ifdef __cplusplus
}
#endif
