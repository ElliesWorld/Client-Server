#include "communication.h"
#include <Arduino.h>

bool communication_init(const char *comparam)
{
    String param(comparam);
    // Initialize the serial port with the specified port and baud rate
    Serial.begin(param.toInt());
    return Serial;
}

bool communication_write(const uint8_t *data, size_t dlen)
{
    // Write data to the serial port
    return (dlen == Serial.write(data, dlen));
}

size_t communication_read(uint8_t *buf, size_t blen)
{
    while (0 == Serial.available())
    {
        ;
    }
    // Read available data
    return Serial.readBytes(buf, blen);
}

void communication_close(void)
{
    // Close the serial port
    Serial.end();
}