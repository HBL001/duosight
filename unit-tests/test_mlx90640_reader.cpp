#include "MLX90640Reader.hpp"
#include "i2cUtils.hpp"
#include "mlx90640Transport.h"
#include <vector>
#include <iostream>
#include <cmath>

int main() {
    std::cout << "[TEST] MLX90640 unit-test begin\n";

    duosight::I2cDevice i2c("/dev/i2c-3", 0x33);
    if (!i2c.isOpen()) {
        std::cerr << "[FAIL] I2C open failed" << std::endl;
        return 1;
    }

    mlx90640_set_i2c_device(&i2c);

    duosight::MLX90640Reader sensor(i2c, 0x33);
    if (!sensor.initialize()) {
        std::cerr << "[FAIL] Sensor initialization failed" << std::endl;
        return 1;
    }

    std::vector<float> frame;
    if (!sensor.readFrame(frame)) {
        std::cerr << "[FAIL] Frame read failed" << std::endl;
        return 1;
    }

    if (frame.size() != 768) {
        std::cerr << "[FAIL] Unexpected frame size: " << frame.size() << std::endl;
        return 1;
    }

    float min = frame[0], max = frame[0], sum = 0;
    for (float temp : frame) {
        if (temp < min) min = temp;
        if (temp > max) max = temp;
        sum += temp;
    }
    float avg = sum / frame.size();

    std::cout << "[PASS] Frame acquired: min=" << min << "C max=" << max << "C avg=" << avg << "C\n";

    if (std::isnan(avg) || max < 5.0 || max > 80.0 || avg < -40.0 || avg > 50.0) {
        std::cerr << "[WARN] Temperatures outside expected range" << std::endl;
        return 2;
    }

    std::cout << "[TEST] MLX90640 unit-test completed successfully\n";
    return 0;
}
