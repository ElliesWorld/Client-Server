#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stddef.h>

class Communication
{
public:
    Communication(const char *port, int baudrate);
    bool communication_send(const uint8_t *data, size_t dlen);
    size_t communication_read(uint8_t *buf, size_t blen);
    bool communication_open();
    void communication_close();

private:
    // Add any necessary private members, such as the serial port handle
    int serial_port; // This could be a platform-specific type
};

#endif // COMMUNICATION_H