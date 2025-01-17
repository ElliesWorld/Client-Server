#include <Arduino.h>
#include "communication.h"
#include "session.h"

// Define the Relay pin (specific GPIO pin for controlling the relay)
#define RELAY_PIN GPIO_NUM_32

void setup()
{
    // Initialize serial communication at 115200 baud rate
    communication_init("115200");

    // Set the relay pin as an output and initially turn it off
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
    // Buffer to store response data
    uint8_t response[10];

    // Check if there's any incoming command from the client
    if (Serial.available() > 0)
    {
        // Read the incoming command from the client
        uint8_t command = Serial.read();

        switch (command)
        {
        case CMD_TEMPERATURE: // Client requests temperature // Note: Incoming command to get temperature
        {
            // Read the current temperature
            float temperature = temperatureRead();

            // Copy temperature value to response buffer
            memcpy(response, &temperature, sizeof(float));

            // Send temperature back to the client
            communication_write(response, sizeof(float));
        }
        break;

        case CMD_TOGGLE_RELAY: // Client requests relay toggle // Note: Incoming command to control relay
        {
            // Read the desired relay state from the client
            uint8_t relay_state;
            communication_read(&relay_state, 1);

            // Set the relay to the specified state
            digitalWrite(RELAY_PIN, relay_state);

            // Confirm the current relay state and send back
            response[0] = digitalRead(RELAY_PIN);
            communication_write(response, 1);
        }
        break;

        case CMD_CLOSE: // Client requests session close // Note: Incoming command to terminate communication
        {
            // Close the communication channel
            communication_close();

            // Send confirmation of successful close
            response[0] = STATUS_OKAY;
            communication_write(response, 1);
        }
        break;

        default: // Unknown command from client // Note: Handle unrecognized commands
            // Send an error status if an unrecognized command is received
            response[0] = STATUS_ERROR;
            communication_write(response, 1);
            break;
        }
    }
}