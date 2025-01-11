#include <Arduino.h>
#include "../lib/session/session.h"
#include "../lib/communication/communication.h"

#define RELAY_PIN 32
#define LED_PIN 21

Session secureSession;
Communication communication(Serial);

void setup()
{
    // Initialize Serial
    Serial.begin(115200);
    communication.init();

    // Initialize Pins
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    // Initial State
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

}

void loop()
{
    // Check Session Status
    if (!secureSession.isSessionEstablished())
    {
        // Attempt to Re-establish Session
        secureSession.endSession();
        if (secureSession.establishKeyExchange())
        {
            digitalWrite(LED_PIN, HIGH);
        }
        else
        {
            digitalWrite(LED_PIN, LOW);
        }
    }

    // Handle Incoming Commands
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');

        if (command == "GET_TEMP")
        {
            communication.handleCommand(GET_TEMPERATURE);
        }
        else if (command == "TOGGLE_RELAY")
        {
            communication.handleCommand(TOGGLE_RELAY);
        }
    }
    // Small delay to prevent watchdog timer issues
    delay(10);
}