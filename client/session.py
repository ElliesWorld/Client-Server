import os
import struct
from typing import Optional
from mbedtls import pk, hmac, hashlib, cipher
from communication import Communication

class Session:
    # constants
    COMMANDS = {
        'TEMPERATURE': 0x01,
        'RELAY_TOGGLE': 0x02,
        'SESSION_CLOSE': 0x03
    }

    STATUS_CODES = {
        0x00: "STATUS OKAY",
        0x01: "STATUS ERROR",
        0x02: "STATUS EXPIRED",
    }

    def __init__(self, port: str):
        """
        Initialize secure communication session
        
        Args:
            port (str): Communication port
        """
        self.communication = Communication(port)
        
        # Cryptographic components
        self.secret_key = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"
        self.rsa_key_size = 256
        self.aes_key_size = 32
        self.iv_size = 16

        # Initialize cryptographic contexts
        self.client_rsa = pk.RSA()
        self.server_rsa = None
        self.aes_key = None
        self.session_iv = None
        
        # Session state
        self.session_active = False
        self.session_id = None

    def _generate_hmac(self, data: bytes) -> bytes:
        """Generate HMAC for data integrity"""
        hmac_obj = hmac.new(self.secret_key, digestmod='sha256')
        hmac_obj.update(data)
        return hmac_obj.digest()

    def _verify_hmac(self, data: bytes, received_hmac: bytes) -> bool:
        """Verify data integrity using HMAC"""
        calculated_hmac = self._generate_hmac(data)
        return calculated_hmac == received_hmac

    def key_exchange(self) -> bool:
        """
        Perform secure key exchange with server
        
        Returns:
            bool: Success of key exchange
        """
        try:
            # Generate client RSA key
            self.client_rsa.generate(self.rsa_key_size * 8, 65537)
            
            # Export and send public key
            client_public_key = self.client_rsa.export_public_key()
            self.communication.communication_send(client_public_key)
            
            # Receive server's public key
            server_public_key = self.communication.communication_read(self.rsa_key_size)
            self.server_rsa = pk.RSA().from_DER(server_public_key)
            
            # Generate AES key and IV
            self.aes_key = os.urandom(self.aes_key_size)
            self.session_iv = os.urandom(self.iv_size)
            
            # Encrypt AES key with server's public key
            encrypted_aes_key = self.server_rsa.encrypt(self.aes_key)
            
            # Send encrypted key and IV
            self.communication.communication_send(encrypted_aes_key + self.session_iv)
            
            # Verify session establishment
            response = self.communication.communication_read(len("SESSION_OKAY"))
            self.session_active = (response == b"SESSION_OKAY")
            
            return self.session_active
        
        except Exception as e:
            print(f"Key exchange error: {e}")
            return False

    def send_command(self, command: int, payload: Optional[bytes] = None) -> bytes:
        """
        Send encrypted command to server
        
        Args:
            command (int): Command type
            payload (bytes, optional): Additional payload
        
        Returns:
            bytes: Server response
        """
        
        # Prepare command with session metadata
        full_command = bytes([command])
        if payload:
            full_command += payload
        
        # Pad and encrypt command
        aes_cipher = cipher.AES.new(self.aes_key, cipher.MODE_CBC, self.session_iv)
        padded_command = self._pad(full_command)
        encrypted_command = aes_cipher.encrypt(padded_command)
        
        # Send encrypted command
        self.communication.communication_send(encrypted_command)
        
        # Read and decrypt response
        encrypted_response = self.communication.communication_read(32)  # Adjust size as needed
        decrypted_response = aes_cipher.decrypt(encrypted_response)
        
        return self._unpad(decrypted_response)

    def get_temperature(self) -> float:
        """
        Retrieve temperature from server
        
        Returns:
            float: Temperature value
        """
        try:
            response = self.send_command(self.COMMANDS['TEMPERATURE'])
            return struct.unpack('f', response[:4])[0]
        except Exception as e:
            print(f"Temperature retrieval error: {e}")
            return float('error')

    def toggle_relay(self) -> bool:
        """
        Toggle relay on server
        
        Returns:
            bool: Relay state
        """
        try:
            response = self.send_command(self.COMMANDS['RELAY_TOGGLE'])
            return bool(response[0])
        except Exception as e:
            print(f"Relay toggle error: {e}")
            return False

    def _pad(self, data: bytes) -> bytes:
        """PKCS7 padding"""
        padding_length = cipher.AES.block_size - (len(data) % cipher.AES.block_size)
        return data + bytes([padding_length] * padding_length)

    def _unpad(self, data: bytes) -> bytes:
        """Remove PKCS7 padding"""
        padding_length = data[-1]
        return data[:-padding_length]

    def close_session(self):
        """Close and clean up session"""
        try:
            # Send session close command
            self.send_command(self.COMMANDS['SESSION_CLOSE'])
        except Exception:
            pass
        finally:
            # Reset session state
            self.session_active = False
            self.aes_key = None
            self.session_iv = None
            self.communication.close()