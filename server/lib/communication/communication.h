#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stddef.h>

bool communication_init(const char *comparam);
bool communication_write(const uint8_t *data, size_t dlen);
size_t communication_read(uint8_t *buf, size_t blen);
void communication_close(void);

#endif // COMMUNICATION_H