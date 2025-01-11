#include "communication.h"
//#include <Arduino.h>

Communication::Communication(HardwareSerial &serial) : serialPort(serial) {}

bool Communication::init()
{
    serialPort.begin(115200); // Default baud rate
    return true;
}

bool Communication::handleCommand(CommandType cmd)
{
    switch (cmd)
    {
    case GET_TEMPERATURE:
    {
        float temp = readCoreTemperature();
        return sendTemperatureResponse(temp);
    }

    case TOGGLE_RELAY:
    {
        toggleRelay();
        return sendRelayStateResponse(digitalRead(32) == HIGH);
    }

    default:
        return false;
    }
}

bool Communication::sendMessage(const uint8_t *message, size_t length)
{
    if (!serialPort)
        return false;

    // Send message length first
    serialPort.write((uint8_t *)&length, sizeof(size_t));

    // Send actual message
    serialPort.write(message, length);
    serialPort.flush();
    return true;
}

bool Communication::receiveMessage(uint8_t *buffer, size_t *length)
{
    if (!serialPort.available())
        return false;

    // Receive message length
    size_t expectedLength;
    serialPort.readBytes((uint8_t *)&expectedLength, sizeof(size_t));

    // Receive message
    *length = serialPort.readBytes(buffer, expectedLength);

    return *length == expectedLength;
}

void Communication::close()
{
    serialPort.end();
}

float Communication::readCoreTemperature()
{
    return temperatureRead();
}

void Communication::toggleRelay()
{
    static bool relayState = false;
    relayState = !relayState;
    digitalWrite(32, relayState ? HIGH : LOW);
}

bool Communication::sendTemperatureResponse(float temperature)
{
    // Convert float to bytes
    uint8_t tempBytes[sizeof(float)];
    memcpy(tempBytes, &temperature, sizeof(float));
    return sendMessage(tempBytes, sizeof(float));
}

bool Communication::sendRelayStateResponse(bool state)
{
    uint8_t stateBytes[1] = {state ? 1 : 0};
    return sendMessage(stateBytes, 1);
}