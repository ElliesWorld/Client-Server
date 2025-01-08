#import time, random, struct, serial, base64
#from mbedtls import pk, cipher, hmac, hashlib
import os
import struct
import serial
from mbedtls import pk, cipher, hmac, hashlib

HMAC_SECRET_KEY = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"
AES_KEY_SIZE = 32
AES_IV_SIZE = 16
RSA_KEY_SIZE = 2048
DEFAULT_BAUD_RATE = 115200
HMAC_DIGEST_SIZE = 32  # SHA256 digest size

class SecureCommunication:

    def __init__(self, port, baud_rate=DEFAULT_BAUD_RATE):
        self.serial_port = serial.Serial(port, baud_rate, timeout=1)
        self.rsa_keys = self._generate_rsa_keys()
        self.aes_key = None
        self.aes_iv = None
        self.session_id = None

    def _generate_rsa_keys(self):
        """Generate RSA-2048 key pair"""
        rsa_keys = pk.RSA()
        rsa_keys.generate(RSA_KEY_SIZE, 65537)
        return rsa_keys

    def _generate_hmac(self, message):
        """Generate HMAC-SHA256 for message authentication"""
        hash_mac = hmac.new(HMAC_SECRET_KEY, digestmod="SHA256")
        hash_mac.update(message)
        return hash_mac.digest()

    def _encrypt_with_hmac(self, message):
        """Encrypt message and generate HMAC"""
        # Ensure AES key and IV are established
        if not self.aes_key or not self.aes_iv:
            raise RuntimeError("AES key or IV not established")

        # Create AES cipher
        aes = cipher.AES.new(self.aes_key, cipher.MODE_CBC, self.aes_iv)

        # Pad message
        padding_length = cipher.AES.block_size - (len(message) % cipher.AES.block_size)
        padded_message = message + bytes([padding_length] * padding_length)

        # Encrypt message
        encrypted_message = aes.encrypt(padded_message)

        # Generate HMAC
        hmac_digest = self._generate_hmac(encrypted_message)

        return encrypted_message, hmac_digest

    def _decrypt_with_hmac_verification(self, encrypted_message, received_hmac):
        """Decrypt message after HMAC verification"""
        # Verify HMAC
        computed_hmac = self._generate_hmac(encrypted_message)
        if computed_hmac != received_hmac:
            raise ValueError("HMAC verification failed")

        # Decrypt message
        aes = cipher.AES.new(self.aes_key, cipher.MODE_CBC, self.aes_iv)
        decrypted = aes.decrypt(encrypted_message)

        # Remove padding
        padding_length = decrypted[-1]
        return decrypted[:-padding_length]

    def establish_session(self):
        """Establish a secure communication session"""
        try:
            # 1. Send client public key
            client_pubkey_der = self.rsa_keys.export_key(format="DER", public=True)
            
            # Generate HMAC for public key
            client_pubkey_hmac = self._generate_hmac(client_pubkey_der)
            
            # Send public key with HMAC
            self.serial_port.write(client_pubkey_der + client_pubkey_hmac)

            # 2. Receive server's encrypted public key and HMAC
            server_encrypted_pubkey = self.serial_port.read(RSA_KEY_SIZE // 8)
            server_pubkey_hmac = self.serial_port.read(32)  # HMAC-SHA256 digest size

            # 3. Verify HMAC of server's public key
            computed_hmac = self._generate_hmac(server_encrypted_pubkey)
            if computed_hmac != server_pubkey_hmac:
                raise ValueError("Server public key HMAC verification failed")

            # 4. Decrypt server's public key
            server_pubkey = self.rsa_keys.decrypt(server_encrypted_pubkey)

            # 5. Generate new client RSA key pair
            self.rsa_keys = self._generate_rsa_keys()
            new_client_pubkey = self.rsa_keys.export_public_key()

            # 6. Generate AES key and IV
            self.aes_key = os.urandom(AES_KEY_SIZE)
            self.aes_iv = os.urandom(AES_IV_SIZE)

            # 7. Encrypt AES key and IV with server's public key
            encrypted_secrets = self.rsa_keys.encrypt(self.aes_key + self.aes_iv)

            # 8. Prepare final handshake message
            final_message = new_client_pubkey + encrypted_secrets
            final_hmac = self._generate_hmac(final_message)

            # 9. Send final message
            self.serial_port.write(final_message + final_hmac)

            # 10. Receive session confirmation
            session_response = self.serial_port.read(RSA_KEY_SIZE // 8)
            session_hmac = self.serial_port.read(32)

            # 11. Verify session response
            self._decrypt_with_hmac_verification(session_response, session_hmac)

            # Session established successfully
            return True

        except Exception as e:
            print(f"Session establishment error: {e}")
            return False

    def get_temperature(self):
        """Retrieve temperature from server"""
        # Implement temperature retrieval with encryption and HMAC
        pass

    def toggle_relay(self):
        """Toggle relay on server"""
        # Implement relay toggle with encryption and HMAC
        pass
