#include "session.h"
#include <Arduino.h>
#include "communication.h"
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

enum
{
    STATUS_OKAY,
    STATUS_ERROR,
    STATUS_EXPIRED,
    STATUS_HASH_ERROR,
    STATUS_BAD_REQUEST,
    STATUS_INVALID_SESSION
};

constexpr int AES_SIZE{32};
constexpr int DER_SIZE{294};
constexpr int RSA_SIZE{256};
constexpr int HASH_SIZE{32};
constexpr int EXPONENT{65537};
constexpr int KEEP_ALIVE{60000};
constexpr int AES_BLOCK_SIZE{16};

static mbedtls_aes_context aes_ctx;
static mbedtls_md_context_t hmac_ctx;
static mbedtls_pk_context client_ctx;
static mbedtls_pk_context server_ctx;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

static uint64_t session_id{0};
static uint32_t prev_access{0};
static uint8_t aes_key[AES_SIZE]{0};
static uint8_t enc_iv[AES_BLOCK_SIZE]{0};
static uint8_t dec_iv[AES_BLOCK_SIZE]{0};
static uint8_t buffer[DER_SIZE + RSA_SIZE] = {0};
static const uint8_t secret_key[HASH_SIZE] = {0x29, 0x49, 0xde, 0xc2, 0x3e, 0x1e, 0x34, 0xb5, 0x2d, 0x22, 0xb5,
                                              0xba, 0x4c, 0x34, 0x23, 0x3a, 0x9d, 0x3f, 0xe2, 0x97, 0x14, 0xbe,
                                              0x24, 0x62, 0x81, 0x0c, 0x86, 0xb1, 0xf6, 0x92, 0x54, 0xd6};

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