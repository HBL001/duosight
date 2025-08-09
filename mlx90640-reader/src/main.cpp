/**
 * @file main.cpp
 * @brief Live thermal image viewer for MLX90640 sensor using Qt5.
 *
 * (c) 2025 Highland Biosciences
 * Author: Dr Richard Day
 * Email: richard_day@highlandbiosciences.com
 *
 * Summary:
 *   This standalone GUI test acquires thermal frames from the MLX90640
 *   infrared sensor over I2C and displays them as a color-mapped 32x24
 *   image using Qt5. The display updates in real time (~2 FPS), and
 *   min/max/average temperature statistics are shown below the image.
 *
 *   It uses the DuoSight I2cDevice class and MLX90640Reader wrapper
 *   to communicate with the sensor, and renders thermal data using a
 *   simple blue-to-red linear gradient.
 *
 *   Intended for hardware validation and GUI integration testing.
 */

#include <iostream>
#include <algorithm>
#include <numeric>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include "MLX90640Reader.hpp"
#include "MLX90640Regs.hpp"
#include "i2cUtils.hpp"
#include "mlx90640Transport.h"

QRgb mapTemperatureToColor(float temp, float minT, float maxT) {
    float t = (temp - minT) / (maxT - minT);
    t = std::clamp(t, 0.0f, 1.0f);
    int r = static_cast<int>(255 * t);
    int b = static_cast<int>(255 * (1.0f - t));
    return qRgb(r, 0, b);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // open I2C bus, register MLX90640 device  0x33, makes a handle
    duosight::I2cDevice bus(duosight::Bus::DEV, duosight::Bus::SLAVE_ADDR);
    if (!bus.isOpen()) {
        qCritical("❌ I²C open failed (/dev/i2c-3 @ 0x33)");
        return 1;
    }

    // initialise the reader
    duosight::MLX90640Reader sensor(bus, duosight::Bus::SLAVE_ADDR);
    if (!sensor.initialize()) {
        qCritical("❌ Sensor init failed");
        return 1;
    }

    // GUI layout
    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *imageLabel = new QLabel;
    QLabel *infoLabel = new QLabel("📷 Waiting for data...");

    imageLabel->setMinimumSize(320, 240);  // Upscale for visibility
    imageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(imageLabel);
    layout->addWidget(infoLabel);

    window.setLayout(layout);
    window.setWindowTitle("MLX90640 Live Viewer");
    window.show();

    // Live frame update
    QTimer *timer = new QTimer;
    QObject::connect(timer, &QTimer::timeout, [&]() {
        std::vector<float> frame;

        if (!sensor.readFrame(frame)) {
            infoLabel->setText("❌ Frame read failed");
            return;
        }

        float minT = *std::min_element(frame.begin(), frame.end());
        float maxT = *std::max_element(frame.begin(), frame.end());
        float avgT = std::accumulate(frame.begin(), frame.end(), 0.0f) / frame.size();

        QImage img(duosight::Geometry::WIDTH,
                duosight::Geometry::HEIGHT,
                QImage::Format_RGB888);

        // Optional: clear to a known background
        img.fill(qRgb(0, 0, 0));

        for (int i = 0; i < duosight::Geometry::HEIGHT; ++i) {
            for (int j = 0; j < duosight::Geometry::WIDTH; ++j) {
                float t = frame[i * duosight::Geometry::WIDTH + j];
                img.setPixel(j, i, mapTemperatureToColor(t, minT, maxT));
            }
        }

        QPixmap pixmap = QPixmap::fromImage(img.scaled(320, 240));
        imageLabel->setPixmap(pixmap);
        infoLabel->setText(QString("🌡️ Min: %1 °C | Max: %2 °C | Avg: %3 °C")
            .arg(minT, 0, 'f', 2)
            .arg(maxT, 0, 'f', 2)
            .arg(avgT, 0, 'f', 2));
    });
    timer->start(500);  // ~2 fps for the moment

    return app.exec();
}
