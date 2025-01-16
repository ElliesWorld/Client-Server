#include <Arduino.h>
#include "communication.h"
#include "session.h"

// Define the Relay pin
#define RELAY_PIN GPIO_NUM_32

void setup()
{
    // Initialize serial communication
    communication_init("115200");

    // Initialize Relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
    // Protocol command variables
    uint8_t command;
    uint8_t response[10];

    // Non-blocking check for incoming command
    if (Serial.available() > 0)
    {
        command = Serial.read();

        switch (command)
        {
        case Session::CMD_TEMPERATURE: // Get Temperature
        {
            float temperature = temperatureRead();
            // Convert float to bytes
            memcpy(response, &temperature, sizeof(float));
            communication_write(response, sizeof(float));
        }
        break;

        case Session::CMD_TOGGLE_RELAY: // Toggle Relay
        {
            uint8_t relay_state;
            communication_read(&relay_state, 1);

            // Toggle relay
            digitalWrite(RELAY_PIN, relay_state);

            // Confirm relay state
            response[0] = digitalRead(RELAY_PIN);
            communication_write(response, 1);
        }
        break;

        default:
            // Unknown command
            response[0] = Session::STATUS_ERROR; // Error status
            communication_write(response, 1);
            break;
        }
    }
}