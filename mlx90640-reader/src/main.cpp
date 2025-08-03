#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

#include "MLX90640Reader.h"
#include "i2cUtils.h"
#include "mlx90640Transport.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set up I2C
    duosight::I2cDevice i2c("/dev/i2c-3", 0x33);
    if (!i2c.isOpen()) {
        qCritical("❌ I2C open failed (/dev/i2c-3 @ 0x33)");
        return 1;
    }

    mlx90640_set_i2c_device(&i2c);

    duosight::MLX90640Reader sensor("/dev/i2c-3", 0x33);
    if (!sensor.initialize()) {
        qCritical("❌ Sensor init failed");
        return 1;
    }

    std::vector<float> frame;
    if (!sensor.readFrame(frame)) {
        qCritical("❌ Frame read failed");
        return 1;
    }

    // Compute stats
    float minT = *std::min_element(frame.begin(), frame.end());
    float maxT = *std::max_element(frame.begin(), frame.end());
    float sum = std::accumulate(frame.begin(), frame.end(), 0.0f);
    float avgT = sum / static_cast<float>(frame.size());

    // GUI
    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *labelTitle = new QLabel("<b>DuoSight MLX90640 Sensor</b>");
    QLabel *labelMin = new QLabel(QString("🌡️ Min: %1 °C").arg(minT, 0, 'f', 2));
    QLabel *labelMax = new QLabel(QString("🌡️ Max: %1 °C").arg(maxT, 0, 'f', 2));
    QLabel *labelAvg = new QLabel(QString("📊 Avg: %1 °C").arg(avgT, 0, 'f', 2));

    layout->addWidget(labelTitle);
    layout->addWidget(labelMin);
    layout->addWidget(labelMax);
    layout->addWidget(labelAvg);

    window.setLayout(layout);
    window.setWindowTitle("MLX90640 Thermal Info");
    window.resize(300, 150);
    window.show();

    return app.exec();
}
