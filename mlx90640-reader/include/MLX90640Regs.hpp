#include <cstdint>

constexpr int WIDTH = 32;
constexpr int HEIGHT = 24;
constexpr uint16_t STATUS_REGISTER = 0x8000;
constexpr uint16_t NEW_DATA_MASK = 0x0008; // Bit 3
constexpr uint16_t SUB_FRAME_MASK = 0x0001; // Bit 0


constexpr int MAXRETRIES = 20;
constexpr int DELAYUS = 5000;

// Emissivity: how efficiently the surface radiates infrared (0.0 to 1.0)
constexpr float emissivity_ = 0.95f;     // good generic value for human skin, clothing, walls, etc.

// Ambient temperature: temperature of the sensor surroundings (in °C)
constexpr float ambientTemp_ = 25.0f;    // reasonable indoor lab/workshop default

