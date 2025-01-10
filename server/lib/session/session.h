#ifndef SESSION_H
#define SESSION_H

#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/hmac.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

// Function declarations
void setupSession();
void loopSession();
bool establishSession(uint8_t *input_buffer, size_t length);
void processCommand(uint8_t *input_buffer, size_t length);
void terminateSession();
void computeHMAC(const uint8_t *data, size_t length, uint8_t *output);
void decryptAES(const uint8_t *input, size_t length, uint8_t *output, size_t &output_length);
void encryptAES(const uint8_t *input, size_t length, uint8_t *output, size_t &output_length);
void sendEncryptedResponse(const char *response);
float getCoreTemperature();
void toggleRelay();

#endif // SESSION_H
