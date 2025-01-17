#include "session.h"
#include <Arduino.h>
#include "communication.h"

static uint64_t session_id{0};

int session_init(const char *comparam)
{
    communication_init(comparam);
    return STATUS_OKAY;
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

    uint8_t buffer[1 + sizeof(session_id)]{STATUS_OKAY};
    memcpy(buffer + 1, &session_id, sizeof(session_id));
    return (communication_write(buffer, sizeof(buffer)) ? STATUS_OKAY : STATUS_ERROR);
}

int session_request(void)
{
    int request = SESSION_BAD_REQUEST;
    uint8_t buffer[1 + sizeof(session_id)]{0};

    size_t length = communication_read(buffer, sizeof(buffer));

    if (length == 1)
    {
        request = buffer[0];
    }
    else if (length == 9)
    {
        if (0 == memcmp(&session_id, buffer + 1, sizeof(session_id)))
        {
            request = buffer[0];
        }
    }
    else
    {
        ;
    }

    return request;
}

int session_send_error(void)
{
    uint8_t status{STATUS_ERROR};
    return (communication_write(&status, sizeof(status)) ? STATUS_OKAY : STATUS_ERROR);
}

int session_send_temperature(float temp)
{
    uint8_t buffer[1 + sizeof(temp)]{STATUS_OKAY};
    memcpy(buffer + 1, &temp, sizeof(temp));
    return (communication_write(buffer, sizeof(buffer)) ? STATUS_OKAY : STATUS_ERROR);
}

int session_send_relay_state(uint8_t state)
{
    uint8_t buffer[1 + sizeof(state)]{STATUS_OKAY, state};
    return (communication_write(buffer, sizeof(buffer)) ? STATUS_OKAY : STATUS_ERROR);
}

int session_close(void)
{
    session_id = 0;
    uint8_t status{STATUS_OKAY};
    return (communication_write(&status, sizeof(status)) ? STATUS_OKAY : STATUS_ERROR);
}