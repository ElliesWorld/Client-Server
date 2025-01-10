#include "communication/communication.h"
#include "session/session.h"
#include <iostream>
#include <vector>

int main()
{
    SerialCommunication comm("/dev/ttyUSB0", 9600);
    Session session;

    while (true)
    {
        // Wait for client data
        std::vector<uint8_t> client_data = comm.read(256);

        if (!client_data.empty())
        {
            std::vector<uint8_t> response;
            if (session.establish(client_data, response))
            {
                comm.write(response);
                std::cout << "Session established." << std::endl;
            }
            else
            {
                std::cout << "Session establishment failed." << std::endl;
            }
        }
    }

    comm.close();
    return 0;
}
