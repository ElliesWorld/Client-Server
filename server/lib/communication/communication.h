#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include <cstddef>

enum CommandType
{
    GET_TEMPERATURE,
    TOGGLE_RELAY
};

class Communication
{
public:
    Communication(HardwareSerial &serial);

    bool init();
    bool sendMessage(const uint8_t *message, size_t length);
    bool receiveMessage(uint8_t *buffer, size_t *length);
    void close();

    bool handleCommand(CommandType cmd);
    bool sendTemperatureResponse(float temperature);
    bool sendRelayStateResponse(bool state);

private:
    HardwareSerial &serialPort;

    float readCoreTemperature();
    void toggleRelay();
};

#endif // COMMUNICATION_H