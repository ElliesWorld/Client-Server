#ifndef SESSION_H
#define SESSION_H

#include <cstdint>
// Command types
constexpr int SESSION_ESTABLISH = 0;
constexpr int SESSION_TEMPERATURE = 1;
constexpr int SESSION_TOGGLE_RELAY = 2;
constexpr int SESSION_CLOSE = 3;
constexpr int SESSION_BAD_REQUEST = 4;

// Status codes
constexpr int STATUS_OKAY = 0;
constexpr int STATUS_ERROR = 1;

int session_init(const char *comparam);

int session_establish(void);

int session_request(void);

int session_send_error(void);

int session_send_temperature(float temp);

int session_send_relay_state(uint8_t state);

int session_close(void);

#endif // SESSION_H