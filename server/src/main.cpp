#include <Arduino.h>
#include "session.h"

// Define the LED and Relay pins
#define LED_PIN 21
#define RELAY_PIN 32

Session secureSession("/dev/ttyUSB0");

float readTemperature()
{
    // ESP32 temperature reading
    return temperatureRead();
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
    size_t bytes_read = secureSession.receive_response(command_buffer, sizeof(command_buffer));

    if (bytes_read > 0)
    {
        // Process the command
        switch (command_buffer[0])
        {
        case SESSION_GET_TEMP:
            // Handle temperature request
            float temperature = readTemperature(); // Read the core temperature
            secureSession.send_command(reinterpret_cast<uint8_t *>(&temperature), sizeof(temperature));
            break;

        case SESSION_TOGGLE_LED:
            // Toggle LED state
            static bool led_state = false;
            led_state = !led_state;
            digitalWrite(LED_PIN, led_state ? HIGH : LOW);
            secureSession.send_command(reinterpret_cast<uint8_t *>(&led_state), sizeof(led_state));
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

    // Small delay to prevent watchdog timer issues
    delay(10);
}
