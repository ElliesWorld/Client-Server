#ifndef SESSION_H
#define SESSION_H

#include <Arduino.h>

class Session
{
public:
    // Command types
    static constexpr uint8_t CMD_CLOSE = 0;
    static constexpr uint8_t CMD_TEMPERATURE = 1;
    static constexpr uint8_t CMD_TOGGLE_RELAY = 2;

    // Status codes
    static constexpr uint8_t STATUS_OKAY = 0;
    static constexpr uint8_t STATUS_ERROR = 1;

    // Function to read temperature 
    static bool sendTemperature(float temp) { return true;}
};

#endif // SESSION_H