import serial
import struct

class Communication:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate)
        self.session_active = False
        self.port = port  # Store port for reference

    def communication_send(self, buffer: bytes):
        return self.ser.write(buffer)

    def communication_read(self, size: int) -> bytes:
        return self.ser.read(size)

    def close(self):
        self.ser.close()
        self.session_active = False

    def is_session_active(self):
        return self.session_active

    def establish_session(self):
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
        
        # Mock responses for testing
        if command == b"GET_TEMP":
            return struct.pack('f', 25.5)  # Mock temperature of 25.5°C
        elif command == b"TOGGLE_RELAY":
            return struct.pack('B', 1)  # Mock relay ON state
        
        return self.communication_read(size)