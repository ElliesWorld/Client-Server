#include "communication.h"
#include <serial/serial.h> // Include a serial library for C++
#include <iostream>

class SerialCommunication::Impl
{
public:
    serial::Serial serial_port;

    Impl(const std::string &port, unsigned long baud_rate)
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
};

SerialCommunication::SerialCommunication(const std::string &port, unsigned long baud_rate)
    : pImpl(new Impl(port, baud_rate)) {}

void SerialCommunication::write(const unsigned char *data, size_t length)
{
    if (!pImpl->serial_port.isOpen())
    {
        std::cerr << "Serial port is not open!" << std::endl;
        return;
    }
    pImpl->serial_port.write(data, length);
}

std::vector<unsigned char> SerialCommunication::read(size_t length)
{
    std::vector<unsigned char> buffer(length);
    if (!pImpl->serial_port.isOpen())
    {
        std::cerr << "Serial port is not open!" << std::endl;
        return buffer; // Return empty buffer
    }
    pImpl->serial_port.read(buffer.data(), length);
    return buffer;
}

void SerialCommunication::close()
{
    if (pImpl->serial_port.isOpen())
    {
        pImpl->serial_port.close();
        std::cout << "Serial port closed." << std::endl;
    }
}
