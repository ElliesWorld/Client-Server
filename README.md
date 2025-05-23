# Secure Client-Server Communication Project

This project demonstrates secure communication between a Python-based client application and a C++ server, leveraging a robust suite of cryptographic protocols: SHA256, HMAC-SHA256, AES-256, and RSA-2048. The client provides a user-friendly graphical interface built with PyQt6, communicating with the server over a serial connection.

## Key Features

The client application enables users to:

* **Configure Connection:** Specify the baud rate and serial port as command-line arguments.
* **Manage Sessions:** Establish and terminate secure communication sessions with the server.
* **Monitor Temperature:** Retrieve the core temperature of the ESP32 microcontroller (server).
* **Control Hardware:** Toggle a relay connected to pin 32 of the server.
* **Track Activity:** View logs of states, statuses, and request outcomes, with the ability to clear the log.
* **Context-Aware UI:** The temperature and relay control buttons are automatically disabled when no session is active.
* **Dynamic Session Button:** The session button label dynamically updates to "Establish Session" when no session exists and "Close Session" when a session is active.
* **Session Timeout:** Inactive server sessions automatically expire after one minute of no communication.

## Technologies Used

* **Client:** Python, PyQt6
* **Server:** C++, PlatformIO, (intended for an ESP32 microcontroller)
* **Communication:** Serial (UART)
* **Security:** SHA256, HMAC-SHA256, AES-256, RSA-2048

---

![image](https://github.com/user-attachments/assets/6c58eede-f469-465e-97c1-8e0e49b7369f)

