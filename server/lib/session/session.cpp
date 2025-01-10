#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/hmac.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include "communication.h"
#include <mbedtls/rsa.h>

#define RSA_KEY_SIZE 2048
#define AES_KEY_SIZE 32
#define AES_IV_SIZE 16
#define HMAC_KEY_SIZE 32
#define SESSION_TIMEOUT_MS 60000

// Shared HMAC secret key
const char *HMAC_SECRET_KEY = "Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+";

// Global variables
mbedtls_pk_context rsa_ctx;
uint8_t aes_key[AES_KEY_SIZE];
uint8_t aes_iv[AES_IV_SIZE];
bool session_active = false;
unsigned long last_activity_time = 0;

void setup()
{
    Serial.begin(115200);

    // Initialize RSA
    mbedtls_pk_init(&rsa_ctx);
    mbedtls_pk_setup(&rsa_ctx, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));

    // Generate RSA keys
    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(rsa_ctx);
    mbedtls_rsa_gen_key(rsa, mbedtls_ctr_drbg_random, nullptr, RSA_KEY_SIZE, 65537);

    Serial.println("Server initialized.");
}

void loop()
{
    if (session_active && (millis() - last_activity_time > SESSION_TIMEOUT_MS))
    {
        terminateSession();
        Serial.println("Session expired.");
    }

    if (Serial.available())
    {
        handleClientRequest();
    }
}

void terminateSession()
{
    session_active = false;
    memset(aes_key, 0, AES_KEY_SIZE);
    memset(aes_iv, 0, AES_IV_SIZE);
}

void handleClientRequest()
{
    // Read client data (simplified example)
    size_t input_length = Serial.available();
    uint8_t input_buffer[input_length];
    Serial.readBytes(input_buffer, input_length);

    if (!session_active)
    {
        // Handle session establishment
        if (!establishSession(input_buffer, input_length))
        {
            Serial.println("Session establishment failed.");
        }
    }
    else
    {
        // Separate encrypted message and hex HMAC
        char received_hmac_hex[65];
        memcpy(received_hmac_hex, input_buffer + input_length - 64, 64);
        received_hmac_hex[64] = '\0'; // Null-terminate
        uint8_t encrypted_message[input_length - 64];
        memcpy(encrypted_message, input_buffer, input_length - 64);

        // Decode received hex HMAC into binary
        uint8_t received_hmac[32];
        hexToBinary(received_hmac_hex, received_hmac);

        // Verify HMAC
        uint8_t computed_hmac[32];
        computeHMAC(encrypted_message, input_length - 64, computed_hmac);

        if (memcmp(computed_hmac, received_hmac, 32) != 0)
        {
            Serial.println("HMAC verification failed.");
            return;
        }

        // Process the command
        uint8_t decrypted_message[256];
        size_t decrypted_length;
        decryptAES(encrypted_message, input_length - 64, decrypted_message, decrypted_length);
        processCommand(decrypted_message, decrypted_length);
    }

    last_activity_time = millis();
}

// Helper function to convert hex to binary
void hexToBinary(const char *hex, uint8_t *binary)
{
    for (size_t i = 0; i < 32; ++i)
    {
        sscanf(&hex[i * 2], "%2hhx", &binary[i]);
    }
}

bool establishSession(uint8_t *input_buffer, size_t length)
{
    // Step 1: Decrypt the client's public key
    uint8_t client_public_key[256]; // Adjust size as needed
    size_t decrypted_length = sizeof(client_public_key);

    int ret = mbedtls_rsa_rsaes_oaep_decrypt(
        mbedtls_pk_rsa(rsa_ctx), mbedtls_ctr_drbg_random, nullptr,
        MBEDTLS_RSA_PRIVATE, nullptr,
        input_buffer, length, client_public_key, &decrypted_length, sizeof(client_public_key));

    if (ret != 0)
    {
        Serial.println("Failed to decrypt client public key.");
        return false;
    }

    Serial.println("Client public key decrypted successfully.");

    // Step 2: Generate AES key and IV
    esp_fill_random(aes_key, AES_KEY_SIZE);
    esp_fill_random(aes_iv, AES_IV_SIZE);

    // Combine AES key and IV into a single buffer for transmission
    uint8_t key_iv_combined[AES_KEY_SIZE + AES_IV_SIZE];
    memcpy(key_iv_combined, aes_key, AES_KEY_SIZE);
    memcpy(key_iv_combined + AES_KEY_SIZE, aes_iv, AES_IV_SIZE);

    // Step 3: Compute HMAC for the key/IV combination
    uint8_t hmac[32];
    computeHMAC(key_iv_combined, sizeof(key_iv_combined), hmac);

    // Convert HMAC to hex
    char hmac_hex[65];
    hmacToHex(hmac, 32, hmac_hex);

    // Step 4: Encrypt the key/IV using RSA
    uint8_t encrypted_secrets[RSA_KEY_SIZE];
    mbedtls_rsa_rsaes_oaep_encrypt(
        mbedtls_pk_rsa(rsa_ctx), mbedtls_ctr_drbg_random, nullptr,
        MBEDTLS_RSA_PUBLIC, nullptr, 0,
        sizeof(key_iv_combined), key_iv_combined, encrypted_secrets);

    // Step 5: Transmit encrypted secrets and HMAC
    Serial.write(encrypted_secrets, RSA_KEY_SIZE);
    Serial.write(hmac_hex, 64); // Transmit HMAC as hex

    session_active = true;
    Serial.println("Session established.");
    return true;
}

/*// Validate the DER public key
bool Session::validateDERPublicKey(const uint8_t *key, size_t length)
{
    mbedtls_pk_context key_ctx;
    mbedtls_pk_init(&key_ctx);

    int ret = mbedtls_pk_parse_public_key(&key_ctx, key, length);
    if (ret != 0)
    {
        mbedtls_pk_free(&key_ctx);
        return false;
    }

    mbedtls_pk_free(&key_ctx);
    return true;
}*/

void processCommand(uint8_t *input_buffer, size_t length)
{
    uint8_t decrypted_message[256];
    uint8_t hmac_received[32];
    size_t decrypted_length;

    // Separate the encrypted message and HMAC
    memcpy(hmac_received, input_buffer + length - 32, 32);
    uint8_t *encrypted_message = input_buffer;

    // Verify HMAC
    uint8_t computed_hmac[32];
    computeHMAC(encrypted_message, length - 32, computed_hmac);
    if (memcmp(computed_hmac, hmac_received, 32) != 0)
    {
        Serial.println("HMAC verification failed.");
        return;
    }

    // Decrypt the message
    decryptAES(encrypted_message, length - 32, decrypted_message, decrypted_length);

    // Process the command
    String command((char *)decrypted_message);
    if (command.startsWith("GET_TEMP"))
    {
        float temperature = getCoreTemperature();
        sendEncryptedResponse(String(temperature).c_str());
    }
    else if (command.startsWith("TOGGLE_RELAY"))
    {
        toggleRelay();
        sendEncryptedResponse("Relay toggled.");
    }
    else
    {
        sendEncryptedResponse("Unknown command.");
    }
}

void computeHMAC(const uint8_t *data, size_t length, uint8_t *output)
{
    mbedtls_md_context_t hmac_ctx;
    mbedtls_md_init(&hmac_ctx);
    mbedtls_md_setup(&hmac_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&hmac_ctx, (const uint8_t *)HMAC_SECRET_KEY, strlen(HMAC_SECRET_KEY));
    mbedtls_md_hmac_update(&hmac_ctx, data, length);
    mbedtls_md_hmac_finish(&hmac_ctx, output);
    mbedtls_md_free(&hmac_ctx);
}

// Helper function to convert HMAC to hex
void hmacToHex(const uint8_t *hmac, size_t hmac_size, char *hex_output)
{
    for (size_t i = 0; i < hmac_size; ++i)
    {
        sprintf(&hex_output[i * 2], "%02x", hmac[i]);
    }
}

void decryptAES(const uint8_t *input, size_t length, uint8_t *output, size_t &output_length)
{
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_dec(&aes_ctx, aes_key, AES_KEY_SIZE * 8);

    // AES CBC decryption
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, length, aes_iv, input, output);

    // Remove padding
    output_length = length - output[length - 1];
    mbedtls_aes_free(&aes_ctx);
}

void sendEncryptedResponse(const char *response)
{
    uint8_t encrypted_message[256];
    uint8_t hmac[32];
    char hmac_hex[65]; // Hex representation of HMAC
    size_t encrypted_length;

    encryptAES((const uint8_t *)response, strlen(response), encrypted_message, encrypted_length);
    computeHMAC(encrypted_message, encrypted_length, hmac);

    // Convert HMAC to hex
    hmacToHex(hmac, 32, hmac_hex);

    Serial.write(encrypted_message, encrypted_length);
    Serial.write(hmac_hex, 64); // Send hex representation
}

void encryptAES(const uint8_t *input, size_t length, uint8_t *output, size_t &output_length)
{
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, aes_key, AES_KEY_SIZE * 8);

    // Add padding
    size_t padding = AES_KEY_SIZE - (length % AES_KEY_SIZE);
    uint8_t padded_input[length + padding];
    memcpy(padded_input, input, length);
    memset(padded_input + length, padding, padding);

    // AES CBC encryption
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, length + padding, aes_iv, padded_input, output);
    output_length = length + padding;
    mbedtls_aes_free(&aes_ctx);
}

float getCoreTemperature()
{
    // Read ESP32 core temperature (stub)
    return 45.0; // Example temperature
}

void toggleRelay()
{
    // Toggle relay state on pin 32
    static bool relay_state = false;
    relay_state = !relay_state;
    digitalWrite(32, relay_state);
    Serial.println(relay_state ? "Relay ON" : "Relay OFF");
}
