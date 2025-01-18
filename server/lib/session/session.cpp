#include "session.h"
#include <Arduino.h>
#include "communication.h"

enum
{
    STATUS_OKAY,
    STATUS_ERROR,
    STATUS_EXPIRED,
    STATUS_HASH_ERROR,
    STATUS_BAD_REQUEST,
    STATUS_INVALID_SESSION
};

static uint64_t session_id{0};
static uint32_t prev_access{0};

int session_init(const char *comparam)
{
    return communication_init(comparam) ? SESSION_OKAY : SESSION_ERROR;
}

static int session_write(const uint8_t *buf, size_t size)
{
    uint8_t buffer[1 + sizeof(session_id)]{0};
    memcpy(buffer, buf, (size > sizeof(buffer)) ? sizeof(buffer) : size);
    return (communication_write(buffer, sizeof(buffer)) ? SESSION_OKAY : SESSION_ERROR);
}

int session_establish(void)
{
    session_id = 0;
    uint8_t *ptr{reinterpret_cast<uint8_t *>(&session_id)};

    while (session_id == 0)
    {
        for (int i = 0; i < sizeof(session_id); i++)
        {
            ptr[i] = random(256);
        }
    }

    prev_access = millis();

    uint8_t buffer[1 + sizeof(session_id)]{STATUS_OKAY};
    memcpy(buffer + 1, &session_id, sizeof(session_id));

    return session_write(buffer, sizeof(buffer));
}

int session_request(void)
{
    uint8_t status = STATUS_OKAY;
    int request = SESSION_WARNING;
    uint8_t buffer[1 + sizeof(session_id)]{0};

    size_t length = communication_read(buffer, sizeof(buffer));

    if (length == 1)
    {
        request = SESSION_ESTABLISH;
    }
    else if (length == 9)
    {
        if (session_id != 0)
        {
            uint32_t access = millis();

            if (access - prev_access <= 60000)
            {
                prev_access = access;

                if (0 == memcmp(&session_id, buffer + 1, sizeof(session_id)))
                {
                    switch (buffer[0])
                    {
                    case SESSION_CLOSE:
                    case SESSION_TEMPERATURE:
                    case SESSION_TOGGLE_RELAY:
                        request = buffer[0];
                        break;

                    default:
                        status = STATUS_BAD_REQUEST;
                        break;
                    }
                }
                else
                {
                    status = STATUS_INVALID_SESSION;
                }
            }
            else
            {
                session_id = 0;
                status = STATUS_EXPIRED;
            }
        }
        else
        {
            status = STATUS_INVALID_SESSION;
        }
    }
    else
    {
        status = STATUS_BAD_REQUEST;
    }

    if (request == SESSION_WARNING)
    {
        request = session_write(&status, sizeof(status));
    }

    return request;
}

int session_send_error(void)
{
    uint8_t status{STATUS_ERROR};
    return session_write(&status, sizeof(status));
}

int session_send_temperature(float temp)
{
    uint8_t buffer[1 + sizeof(temp)]{STATUS_OKAY};
    memcpy(buffer + 1, &temp, sizeof(temp));
    return session_write(buffer, sizeof(buffer));
}

int session_send_relay_state(uint8_t state)
{
    uint8_t buffer[1 + sizeof(state)]{STATUS_OKAY, state};
    return session_write(buffer, sizeof(buffer));
}

int session_close(void)
{
    session_id = 0;
    uint8_t status{STATUS_OKAY};
    return session_write(&status, sizeof(status));
}