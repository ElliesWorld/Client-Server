#include <Arduino.h>
#include "session.h"

// Define the Relay pin (specific GPIO pin for controlling the relay)
#define LED_PIN GPIO_NUM_21
#define RELAY_PIN GPIO_NUM_32

static void set_status(int status)
{
    if (status == SESSION_OKAY)
    {
        analogWrite(LED_PIN, 0x00);
    }
    else if (status == SESSION_WARNING)
    {
        analogWrite(LED_PIN, 0x7F);
    }
    else
    {
        analogWrite(LED_PIN, 0xFF);
    }
}

void setup()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    if (SESSION_OKAY != session_init("115200"))
    {
        set_status(SESSION_ERROR);

        while (1)
        {
            ;
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
        break;
    }

    set_status(request);
}