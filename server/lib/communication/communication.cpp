#include "communication.h"
#include "session.h"
#include <iostream>
#include <serial/serial.h> // Include a serial library for C++
#include <Arduino.h>

class SerialCommunication
{
private:
    serial::Serial serial_port; // Serial port object

public:
    SerialCommunication(const std::string &port, unsigned long baud_rate)
        : serial_port(port, baud_rate, serial::Timeout::simpleTimeout(1000))
    {
        if (!serial_port.isOpen())
        {
            std::cerr << "Failed to open serial port!" << std::endl;
        }
        else
        {
            std::cout << "Serial port opened successfully." << std::endl;
        }
    }

    void write(const unsigned char *data, size_t length)
    {
        if (!serial_port.isOpen())
        {
            std::cerr << "Serial port is not open!" << std::endl;
            return;
        }
        serial_port.write(data, length);
    }

    std::vector<unsigned char> read(size_t length)
    {
        std::vector<unsigned char> buffer(length);
        if (!serial_port.isOpen())
        {
            std::cerr << "Serial port is not open!" << std::endl;
            return buffer; // Return empty buffer
        }
        serial_port.read(buffer.data(), length);
        return buffer;
    }

    void close()
    {
        if (serial_port.isOpen())
        {
            serial_port.close();
            std::cout << "Serial port closed." << std::endl;
        }
    }
};