import serial

class Communication:
    def __init__(self, info: str) -> None:
        try:
            port, speed = info.split(':')
            self._port = port
            self._speed = int(speed)
            self._connection = serial.Serial(self._port, self._speed)
        except ValueError:
            raise ValueError("Invalid format for 'info'. Use 'port:speed'.")
        except serial.SerialException as e:
            raise ConnectionError(f"Failed to initialize serial connection: {e}")
    
    def connect(self) -> bool:
        try:
            if not self._connection.is_open:
                self._connection.open()
            return self._connection.is_open
        except serial.SerialException as e:
            print(f"Error connecting to the port: {e}")
            return False

    def disconnect(self) -> None:
        try:
            if self._connection.is_open:
                self._connection.close()
        except serial.SerialException as e:
            print(f"Error disconnecting the port: {e}")
    
    def send(self, data: bytes) -> bool:
        if not self._connection.is_open:
            print("Connection is not open. Unable to send data.")
            return False
        
        try:
            self._connection.reset_output_buffer()
            bytes_written = self._connection.write(data)
            return bytes_written == len(data)
        except serial.SerialException as e:
            print(f"Error while sending data: {e}")
            return False
    
    def receive(self, size: int) -> bytes:
        if not self._connection.is_open:
            print("Connection is not open. Unable to receive data.")
            return b""
        
        try:
            self._connection.reset_input_buffer()
            return self._connection.read(size)
        except serial.SerialException as e:
            print(f"Error while receiving data: {e}")
            return b""

    def __del__(self) -> None:
        self.disconnect()