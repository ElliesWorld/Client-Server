import time, random, struct, base64, serial
from mbedtls import pk, cipher, hmac, hashlib

class SecureCommunication:

    HMAC_SECRET_KEY = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"
    AES_KEY_SIZE = 32
    RSA_KEY_SIZE = 256

    def __init__(self, port, baud_rate):
        """Initialize secure communication"""
        self._serial_port = serial.Serial(port, baud_rate, timeout=1)
        from session import SecureSession
        self.session = SecureSession(self.HMAC_SECRET_KEY)

    def establish_session(self, port, baudrate):
        try:
            self.serial = serial.Serial(port, int(baudrate), timeout=1)
            self.session_active = True
            return True
        except Exception as e:
            print(f"Error: {e}")
            return False

    def terminate_session(self):
        if self.serial:
            self.serial.close()
        self.session_active = False

    def is_session_active(self):
        return self.session_active
    
    def send_command(self, command):
        if not self.session_active:
            return None

        # Generate HMAC for the command
        hmac_gen = hmac.new(self.HMAC_SECRET_KEY, digestmod="SHA256")
        hmac_gen.update(command.encode())
        hmac_value = hmac_gen.digest()

        # Send command + HMAC
        payload = base64.b64encode(command.encode() + hmac_value)
        self.serial.write(payload + b'\n')

        # Read response
        response = self.serial.readline().strip()
        return base64.b64decode(response).decode()

    def get_temperature(self):
        return self.send_command("GET_TEMP")

    def toggle_relay(self):
        return self.send_command("TOGGLE_RELAY")
