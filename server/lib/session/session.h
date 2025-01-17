#ifndef SESSION_H
#define SESSION_H

#include <Arduino.h>

// Command types
constexpr uint8_t CMD_CLOSE = 0;
constexpr uint8_t CMD_TEMPERATURE = 1;
constexpr uint8_t CMD_TOGGLE_RELAY = 2;

// Status codes
constexpr uint8_t STATUS_OKAY = 0;
constexpr uint8_t STATUS_ERROR = 1;

// Function to handle temperature response
bool sendTemperature(float temp);

// Add additional utility functions if necessary
#endif // SESSION_H
