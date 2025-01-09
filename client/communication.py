import serial 
import time

from mbedtls import cipher, hmac

class SecureCommunication:

    def __init__(self, port: str, baud_rate: int):
        self.port = port
        self.baud_rate = baud_rate
#        self.session = SecureSession()  # placeholder 
        self.serial_port = serial.Serial(port, baud_rate, timeout=1)

    def establish_session(self) -> bool:
        """Establish a secure session."""
        return self.session.establish_session()

    def close(self):
        """Close the serial port."""
        if self.serial_port.is_open:
            self.serial_port.close()
            print("Serial port closed.")

    def end_session(self):
        """End the current session."""
        self.session.end_session()

    def is_active(self) -> bool:
        """Check if the session is active."""
        return self.session.is_active()

    def send_message(self, message: bytes):
        """Send a message to the server."""
        if not self.is_active():
            raise RuntimeError("No active session.")

        # Sending the raw message bytes
        self.serial_port.write(message)

    def receive_message(self, size: int) -> bytes:
        """Receive a message from the server."""
        # Reading the specified number of bytes from the serial port
        return self.serial_port.read(size)    