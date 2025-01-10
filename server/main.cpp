#include <Arduino.h>
#include "session.h"
#include "communication.h"

SerialCommunication *serialComm;

void setup()
{
    Serial.begin(115200);
    setupSession();
    serialComm = new SerialCommunication("/dev/ttyUSB0", 115200);
}

void loop()
{
    loopSession();

    if (Serial.available())
    {
        size_t input_length = Serial.available();
        uint8_t input_buffer[input_length];
        Serial.readBytes(input_buffer, input_length);

        // Forward received data to session processing
        processCommand(input_buffer, input_length);
    }
}
