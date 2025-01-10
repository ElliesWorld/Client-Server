#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <string>
#include <vector>

class SerialCommunication
{
public:
    SerialCommunication(const std::string &port, unsigned long baud_rate);
    void write(const unsigned char *data, size_t length);
    std::vector<unsigned char> read(size_t length);
    void close();

private:
    class Impl; // Forward declaration for PImpl idiom
    Impl *pImpl;
};

#endif // COMMUNICATION_H
