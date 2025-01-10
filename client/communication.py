import serial 
import time

from mbedtls import cipher, hmac
from session import SecureCommunication

class Communication:

    def __init__(self, port: str, baud_rate: int):
        self.port = port
        self.baud_rate = baud_rate
        self.serial_port = serial.Serial(port, baud_rate, timeout=1)
        self.session_active = False  # Track session status

    def connect(self):
        """Establish a secure connection."""
        try:
            self.secure_comm = SecureCommunication(self.port, self.baud_rate)
            if self.secure_comm.establish_session():
                print("Secure session established.")
            else:
                print("Failed to establish a secure session.")
        except Exception as e:
            print(f"Connection error: {e}")

    def disconnect(self):
        """Terminate the secure connection."""
        if self.secure_comm:
            self.secure_comm.end_session()
            self.secure_comm = None
            print("Disconnected successfully.")

    def send(self, command: bytes):
        """Send a command securely."""
        if not self.secure_comm or not self.secure_comm.is_session_active():
            print("Session is not active. Please reconnect.")
            return None
        
        try:
            self.secure_comm.check_session_timeout()
            self.secure_comm.send_command(command)
        except Exception as e:
            print(f"Error while sending: {e}")

    def receive(self):
        """Receive a response securely."""
        if not self.secure_comm or not self.secure_comm.is_session_active():
            print("Session is not active. Please reconnect.")
            return None
        
        try:
            self.secure_comm.check_session_timeout()
            return self.secure_comm.receive_response()
        except Exception as e:
            print(f"Error while receiving: {e}")
            return None

    def delete(self):
        """Delete session and resources."""
        self.disconnect()
        self.secure_comm = None
        print("Session deleted.")