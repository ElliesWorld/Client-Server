import serial
import struct

class Communication:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate)
        self.session_active = False
        self.port = port  # Store port for reference

    def communication_send(self, buffer: bytes)-> int:
        return self.ser.write(buffer)

    def communication_read(self, size: int) -> bytes:
        return self.ser.read(size)

    def close(self):
        self.ser.close()
        self.session_active = False

    def is_session_active(self) -> bool:
        return self.session_active

    def establish_session(self) -> bool:
        self.session_active = True
        return True

    # Add these methods to match the GUI expectations
    def send_command(self, command: bytes):
        if not self.session_active:
            raise RuntimeError("Session not active")
        self.communication_send(command)

    def receive_response(self, size: int = 4) -> bytes:
        if not self.session_active:
            raise RuntimeError("Session not active")
        
        return self.communication_read(size)