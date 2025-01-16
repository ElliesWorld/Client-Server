#include "communication.h"
#include <Arduino.h>

Communication::Communication(const char *port, int baudrate)
{
    // Initialize the serial port with the specified port and baud rate
    Serial.begin(baudrate);
}

bool Communication::communication_send(const uint8_t *data, size_t dlen)
{
    // Write data to the serial port
    return (dlen == Serial.write(data, dlen));
}

size_t Communication::communication_read(uint8_t *buf, size_t blen)
{
    while (0 == Serial.available())
    {
        ;
    }

    // Read available data
    return Serial.readBytes(buf, blen);
}

bool Communication::communication_open()
{
    // Open the serial port if it's not already open
    if (!Serial)
    {
        Serial.begin(115200);
    }

    return Serial;
}

void Communication::communication_close()
{
    // Close the serial port
    Serial.end();
}