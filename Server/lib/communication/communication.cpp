#include "communication.h"
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <stdint.h>
#include <string.h>
// AES, HMAC, and RSA initialization

// Shared HMAC key
const uint8_t HMAC_SECRET_KEY[] = "Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+";

bool validate_hmac(const char *message, const uint8_t *received_hmac)
{
    uint8_t generated_hmac[32];
    mbedtls_md_hmac_starts(&hmac_ctx, (const uint8_t *)HMAC_SECRET_KEY, strlen(HMAC_SECRET_KEY));
    mbedtls_md_hmac_update(&hmac_ctx, (const uint8_t *)message, strlen(message));
    mbedtls_md_hmac_finish(&hmac_ctx, generated_hmac);

    return memcmp(received_hmac, generated_hmac, sizeof(generated_hmac)) == 0;
}

void process_command(const char *command)
{
    if (strcmp(command, "GET_TEMP") == 0)
    {
        // Respond with temperature
        send_temperature();
    }
    else if (strcmp(command, "TOGGLE_RELAY") == 0)
    {
        // Toggle relay state
        toggle_relay();
    }
}
