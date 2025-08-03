/**
 * @file i2cUtils.cpp
 * @brief Implementation of I2C utility class for Linux I2C device access.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Implements low-level I2C read, write, and combined transactions using
 *   ioctl() and the Linux I2C driver interface. Used for accessing sensors
 *   such as MLX90640 from user space.
 */

#include "i2cUtils.h"
#include <linux/i2c.h>  
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <iostream>

/* Verdin imx8x plus base station setup
*
* root@verdin-imx8mp-15364294:~# i2cdetect -y 3
*      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
* 00:                         -- -- -- -- -- -- -- --
* 10: -- -- -- -- -- -- -- -- -- -- UU -- -- -- -- --
* 20: -- UU -- -- -- -- -- -- -- -- -- -- -- -- -- --
* 30: -- -- -- 33 -- -- -- -- -- -- -- -- -- -- -- --
* 40: UU -- -- -- -- -- -- -- UU UU 4a 4b -- -- -- UU
* 50: UU -- -- -- -- -- -- UU -- -- -- -- -- -- -- --
* 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
* 70: -- -- -- -- -- -- -- --
*/

namespace duosight {

I2cDevice::I2cDevice(const std::string& devicePath, uint8_t address)
    : fd_(-1), addr_(address)
{
    fd_ = open(devicePath.c_str(), O_RDWR);
    if (fd_ < 0) {
        std::cerr << "[I2C] Failed to open: " << devicePath << std::endl;
        return;
    }

    if (ioctl(fd_, I2C_SLAVE, addr_) < 0) {
        std::cerr << "[I2C] Failed to set slave address: 0x"
                  << std::hex << static_cast<int>(addr_) << std::endl;
        close(fd_);
        fd_ = -1;
    }
}

I2cDevice::~I2cDevice() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool I2cDevice::isOpen() const {
    return fd_ >= 0;
}

bool I2cDevice::writeBytes(const uint8_t* data, size_t length) {
    if (fd_ < 0) return false;
    return write(fd_, data, length) == static_cast<ssize_t>(length);
}

bool I2cDevice::readBytes(uint8_t* buffer, size_t length) {
    if (fd_ < 0) return false;
    return read(fd_, buffer, length) == static_cast<ssize_t>(length);
}

bool I2cDevice::writeThenRead(const uint8_t* txData, size_t txLen, uint8_t* rxData, size_t rxLen) {
    if (fd_ < 0) return false;

    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    messages[0].addr  = addr_;
    messages[0].flags = 0;
    messages[0].len   = txLen;
    messages[0].buf   = const_cast<uint8_t*>(txData);

    messages[1].addr  = addr_;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = rxLen;
    messages[1].buf   = rxData;

    packets.msgs  = messages;
    packets.nmsgs = 2;

    return ioctl(fd_, I2C_RDWR, &packets) >= 0;
}

} // namespace duosight
