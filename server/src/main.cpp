#include <Arduino.h>
#include "session.h"

// Define the Relay pin (specific GPIO pin for controlling the relay)
#define LED_PIN GPIO_NUM_21
#define RELAY_PIN GPIO_NUM_32

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    if (STATUS_OKAY != session_init("115200"))
    {
        while (1)
        {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(500);
        }
    }
}

void loop()
{
    int request = session_request();

    switch (request)
    {
    case SESSION_ESTABLISH:
        request = session_establish();
        break;

    case SESSION_TEMPERATURE:
        request = session_send_temperature(temperatureRead());
        break;

    case SESSION_TOGGLE_RELAY:
    {
        static uint8_t state = LOW;
        state = (state == LOW) ? HIGH : LOW;
        digitalWrite(RELAY_PIN, state);
        request = (state == digitalRead(RELAY_PIN)) ? session_send_relay_state(state) : session_send_error();
    }
    break;
    
    case SESSION_CLOSE:
        request = session_close();
        break;
    default:
        request = STATUS_ERROR;
        (void)session_send_error();
        break;
    }

    digitalWrite(LED_PIN, (request == STATUS_OKAY) ? LOW : HIGH);
}