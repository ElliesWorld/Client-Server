import struct, os
from mbedtls import pk, hmac, hashlib, cipher
from communication import Communication

class Session:
    RESPONSE = b"SESSION_OKAY"
    RSA_SIZE = 256
    EXPONENT = 65537
    SECRET_KEY = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"

    def __init__(self, port):
        self.ser = Communication(port)
        self.client_public_rsa = pk.RSA()
        self.client_public_rsa.generate(self.RSA_SIZE * 8, self.EXPONENT)
        self.server_public_rsa = None
        self.aes_key = None
        self.session_id = None
        self.iv = os.urandom(16) 
        
    def key_exchange(self):
        # Send client's public key to server
        self.ser.communication_send(self.client_public_rsa.export_public_key())
        
        # Receive server's public key
        server_pub_key = self.ser.communication_read(self.RSA_SIZE)
        self.server_public_rsa = pk.RSA().from_DER(server_pub_key)

        # Generate AES key and IV
        self.aes_key = os.urandom(32)  # AES-256
        iv = os.urandom(16)  # AES block size

        # Encrypt AES key with server's public key
        encrypted_aes_key = self.server_public_rsa.encrypt(self.aes_key)

        # Send encrypted AES key and IV to server
        self.ser.communication_send(encrypted_aes_key + iv)

        # Receive session confirmation
        response = self.ser.communication_read(self.RSA_SIZE)
        if response == self.RESPONSE:
            return True
        return False

    def send_command(self, command: bytes):
        # Encrypt command with AES
        aes_cipher = cipher.AES.new(self.aes_key, cipher.MODE_CBC, iv)
        padded_command = self._pad(command)
        encrypted_command = aes_cipher.encrypt(padded_command)

        # Send command
        self.ser.communication_send(encrypted_command)

    def receive_response(self):
        # Read response
        encrypted_response = self.ser.communication_read(32)  # Adjust size as needed
        aes_cipher = cipher.AES.new(self.aes_key, cipher.MODE_CBC, iv)
        decrypted_response = aes_cipher.decrypt(encrypted_response)
        return self._unpad(decrypted_response)

    def _pad(self, data):
        # Pad data to be a multiple of AES block size
        padding_length = cipher.AES.block_size - (len(data) % cipher.AES.block_size)
        return data + bytes([padding_length] * padding_length)

    def _unpad(self, data):
        # Remove padding
        padding_length = data[-1]
        return data[:-padding_length]