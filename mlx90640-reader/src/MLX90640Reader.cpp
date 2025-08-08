/**
 * @file MLX90640Reader.cpp
 * @brief Implementation of MLX90640Reader using Melexis C API and
 * duosight::I2cDevice.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   Provides initialization of the MLX90640 sensor and reading of thermal frame
 * data. Uses the official Melexis API for parameter extraction and temperature
 * conversion.
 */

#include <cstring>
#include <iostream>
#include <unistd.h>

#include "MLX90640Reader.h"
#include "MLX90640_API.h"

namespace duosight {

MLX90640Reader::MLX90640Reader(const std::string &i2cPath, uint8_t address)
    : address_(address) {
  i2c_ = std::make_unique<I2cDevice>(i2cPath, address);
}

MLX90640Reader::~MLX90640Reader() {}

bool MLX90640Reader::initialize() {
  if (!i2c_ || !i2c_->isOpen()) {
    std::cerr << "[MLX90640] I2C device not open\n";
    return false;
  }

  if (MLX90640_DumpEE(address_, eepromData_) != 0) {
    std::cerr << "[MLX90640] Failed to dump EEPROM\n";
    return false;
  }

  if (MLX90640_ExtractParameters(eepromData_, params_) != 0) {
    std::cerr << "[MLX90640] Failed to extract sensor parameters\n";
    return false;
  }

  MLX90640_SetRefreshRate(address_, 0x05); // 8Hz for test

  return true;
}

bool MLX90640Reader::readFrame(std::vector<float> &frameData) {
  if (!i2c_ || !i2c_->isOpen())
    return false;

  uint16_t frame[834];
  frameData.assign(768, 0.0f);

  // The MLX90640 sensor outputs two interleaved subframes (checkerboard
  // pattern).  We must capture both subframes (0 and 1) and run the
  // temperature calculation on each before the image is complete.

  int remaining = 2;       // subframes still needed
  int expectedSubpage = 0; // start by waiting for subpage 0
  int attempts = 40;       // overall retry budget

  while (remaining > 0 && attempts-- > 0) {
    int status = MLX90640_GetFrameData(address_, frame);
    if (status != 0) {
      usleep(1000);
      continue;
    }

    int subpage = MLX90640_GetSubPageNumber(frame);
    if (subpage != expectedSubpage) {
      // Keep waiting for the expected subpage
      usleep(1000);
      continue;
    }

    MLX90640_CalculateTo(frame, params_, 0.95f, 25.0f, frameData.data());
    expectedSubpage = 1 - expectedSubpage;
    --remaining;
  }

  if (remaining != 0) {
    std::cerr << "[MLX90640] Failed to read complete frame after retries\n";
    return false;
  }

  return true;
}

void MLX90640Reader::printSummary(const std::vector<float> &frameData) {
  if (frameData.size() != 768) {
    std::cerr << "[MLX90640] Invalid frame size\n";
    return;
  }

  float minT = frameData[0];
  float maxT = frameData[0];
  float sumT = 0.0f;

  for (float t : frameData) {
    if (t < minT)
      minT = t;
    if (t > maxT)
      maxT = t;
    sumT += t;
  }

  float avgT = sumT / frameData.size();

  std::cout << "[MLX90640] Frame: min=" << minT << "°C max=" << maxT
            << "°C avg=" << avgT << "°C\n";
}

} // namespace duosight
