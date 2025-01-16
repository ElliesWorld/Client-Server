#include <Arduino.h>
#include "session.h"

// Define the LED and Relay pins
#define LED_PIN GPIO_NUM_21
#define RELAY_PIN GPIO_NUM_32
#define ON "ON"
#define OFF "OFF"

Session secureSession("/dev/ttyUSB0");

float readTemperature()
{
    // Ensure temperature reading is working
#ifdef CONFIG_IDF_TARGET_ESP32
    // ESP32 temperature sensor calibration
    return temperatureRead();
#else
    // Fallback or simulated temperature
    return 25.5; // Default temperature
#endif
}

void setup()
{
    // Initialize Serial
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);

    // Initialize the session
    if (!secureSession.key_exchange())
    {
        Serial.println("Session establishment failed.");
        while (1)
        {
            // Blink LED to indicate failure
            digitalWrite(LED_PIN, HIGH);
            delay(500);
            digitalWrite(LED_PIN, LOW);
            delay(500);
        }
    }
    Serial.println("Session established successfully.");
}

void loop()
{
    // Check for incoming commands
    uint8_t command_buffer[32]; // Adjust size as needed
    memset(command_buffer, 0, sizeof(command_buffer));
    size_t bytes_read = secureSession.receive_response(command_buffer, sizeof(command_buffer));

    if (bytes_read > 0)
    {
        // Process the command
        switch (command_buffer[0])
        {
        case SESSION_GET_TEMP:
            // Handle temperature request
            float temperature = temperatureRead(); // Read the core temperature
            secureSession.send_command((const uint8_t *)&temperature, sizeof(float));
            break;

        case SESSION_TOGGLE_RELAY:
            // Explicit relay toggling with debug
            static bool relay_state = false;
            relay_state = !relay_state;
            Serial.print("Relay State: ");
            Serial.println(relay_state);
            // Ensure correct pin and state
            digitalWrite(RELAY_PIN, relay_state ? HIGH : LOW);
            // Send state back
            uint8_t state_byte = relay_state ? 1 : 0;
            secureSession.send_command(&state_byte, sizeof(state_byte));
            break;

        case SESSION_CLOSE:
            // Handle session close
            secureSession.close_session();
            break;

        default:
            Serial.println("Unknown command received.");
            break;
        }
    }
}