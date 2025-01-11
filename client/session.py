import os
import struct
import serial
import time
from mbedtls import pk, cipher, hmac

HMAC_SECRET_KEY = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"
AES_KEY_SIZE = 32
AES_IV_SIZE = 16
RSA_KEY_SIZE = 2048
DEFAULT_BAUD_RATE = 115200
HMAC_DIGEST_SIZE = 32  # SHA256 digest size
SESSION_TIMEOUT = 60  # 1 minute session timeout

class SecureCommunication:
    def __init__(self, port, baud_rate=DEFAULT_BAUD_RATE):
        try:
            self.serial_port = serial.Serial(port, baud_rate, timeout=1)
            self.rsa_keys = self._generate_rsa_keys()
            self.aes_key = None
            self.aes_iv = None
            self.session_id = None
            self.last_activity_time = None
            self._is_active = False
        except Exception as e:
            print(f"Error initializing communication: {e}")
            raise    

    def _generate_rsa_keys(self):
        try:
            """Generate RSA-2048 key pair"""
            rsa_keys = pk.RSA()
            rsa_keys.generate(RSA_KEY_SIZE, 65537)
            return rsa_keys
        except Exception as e:
            print(f"RSA key generation failed: {e}")
            raise

    def _generate_hmac(self, message: bytes) -> str:
        """Generate HMAC-SHA256 for message authentication (hex format)"""
        hash_mac = hmac.new(HMAC_SECRET_KEY, digestmod="SHA256")
        hash_mac.update(message)
        return hash_mac.hexdigest()

    def _encrypt_with_hmac(self, message):
        """Encrypt message and generate HMAC"""
        if not self.aes_key or not self.aes_iv:
            raise RuntimeError("AES key or IV not established")

        aes = cipher.AES.new(self.aes_key, cipher.MODE_CBC, self.aes_iv)

        # Pad message
        padding_length = cipher.AES.block_size - (len(message) % cipher.AES.block_size)
        padded_message = message + bytes([padding_length] * padding_length)

        # Encrypt message
        encrypted_message = aes.encrypt(padded_message)

        # Generate HMAC
        hmac_digest = self._generate_hmac(encrypted_message)

        return encrypted_message, hmac_digest

    def _decrypt_with_hmac_verification(self, encrypted_message: bytes, received_hmac: str) -> bytes:
        """Decrypt message after HMAC verification"""
        computed_hmac = self._generate_hmac(encrypted_message)
        if computed_hmac != received_hmac:
            raise ValueError("HMAC verification failed")

        aes = cipher.AES.new(self.aes_key, cipher.MODE_CBC, self.aes_iv)
        decrypted = aes.decrypt(encrypted_message)

        # Remove padding
        padding_length = decrypted[-1]
        return decrypted[:-padding_length]

    def establish_session(self):
        """Establish a secure communication session"""
        try:
            # Step 1: Send client public key
            client_pubkey_der = self.rsa_keys.export_key(format="DER")
            client_pubkey_hmac = self._generate_hmac(client_pubkey_der).encode('utf-8')
            self.serial_port.write(client_pubkey_der + client_pubkey_hmac)

            # Step 2: Receive server's encrypted public key and HMAC
            server_encrypted_pubkey = self.serial_port.read(RSA_KEY_SIZE // 8)
            server_pubkey_hmac = self.serial_port.read(HMAC_DIGEST_SIZE)

            # Step 3: Verify HMAC of server's public key
            computed_hmac = self._generate_hmac(server_encrypted_pubkey).encode('utf-8')
            if computed_hmac != server_pubkey_hmac:
                raise ValueError("Server public key HMAC verification failed")
            
            # Modify _generate_hmac to return bytes instead of str
            def _generate_hmac(self, message: bytes) -> bytes:
                """Generate HMAC-SHA256 for message authentication (bytes format)"""
                hash_mac = hmac.new(HMAC_SECRET_KEY, digestmod="SHA256")
                hash_mac.update(message)
                return hash_mac.digest()

            # Step 4: Decrypt server's public key
            server_pubkey = self.rsa_keys.decrypt(server_encrypted_pubkey)

            # Step 5: Generate AES key and IV
            self.aes_key = os.urandom(AES_KEY_SIZE)
            self.aes_iv = os.urandom(AES_IV_SIZE)

            # Step 6: Encrypt AES key and IV with server's public key
            rsa_server = pk.RSA()
            rsa_server.import_key(server_pubkey)
            encrypted_secrets = rsa_server.encrypt(self.aes_key + self.aes_iv)

            # Step 7: Prepare final handshake message
            final_message = client_pubkey_der + encrypted_secrets
            final_hmac = self._generate_hmac(final_message)

            # Step 8: Send final message
            self.serial_port.write(final_message + final_hmac)

            # Step 9: Receive session confirmation
            session_response = self.serial_port.read(RSA_KEY_SIZE // 8)
            session_hmac = self.serial_port.read(HMAC_DIGEST_SIZE)

            # Step 10: Verify session response
            self._decrypt_with_hmac_verification(session_response, session_hmac)

            # Session established successfully
            self.last_activity_time = time.time()  # Update last activity time
            return True

            result = self._establish_session_internal()
            self._is_active = result

        except Exception as e:
            print(f"Session establishment error: {e}")
            import traceback
            traceback.print_exc()  
            return False


    def get_temperature(self):
        """Retrieve temperature from server."""
        if not self.aes_key or not self.aes_iv:
            raise RuntimeError("Session not established")
    
        try:
            # Step 1: Prepare the command to get the temperature
            command = b"GET_TEMP"
            encrypted_command, command_hmac = self._encrypt_with_hmac(command)

            # Step 2: Send the encrypted command and HMAC to the server
            self.serial_port.write(encrypted_command + bytes.fromhex(command_hmac))

            # Step 3: Wait for the response from the server
            response = self.serial_port.read(32)  # Adjust size as needed
            response_hmac = self.serial_port.read(HMAC_DIGEST_SIZE)

            # Step 4: Decrypt the response and verify HMAC
            decrypted_response = self._decrypt_with_hmac_verification(response, response_hmac)

            # Step 5: Parse the decrypted response
            if len(decrypted_response) != 4:
                raise ValueError("Invalid response length for temperature")

            # Convert the decrypted bytes to a float (4 bytes for a single-precision float)
            temperature = struct.unpack('f', decrypted_response)[0]

            # Step 6: Return the parsed temperature
            return temperature

        except Exception as e:
            print(f"Error retrieving temperature: {e}")
            return None

    def toggle_relay(self):
        """Toggle relay on server"""
        if not self.aes_key or not self.aes_iv:
            raise RuntimeError("Session not established")

        try:
            # Prepare the command to toggle relay
            command = b"TOGGLE_RELAY"
            encrypted_command, command_hmac = self._encrypt_with_hmac(command)

            # Send the encrypted command and HMAC to the server
            self.serial_port.write(encrypted_command + bytes.fromhex(command_hmac))

            # Wait for the response
            response = self.serial_port.read(32)  # Adjust size as needed
            response_hmac = self.serial_port.read(HMAC_DIGEST_SIZE)

            # Decrypt the response and verify HMAC
            decrypted_response = self._decrypt_with_hmac_verification(response, response_hmac)

            # Assuming the response is a single byte indicating the relay state
            relay_state = struct.unpack('B', decrypted_response)[0]
            return relay_state == 1  # Return True if relay is ON, False if OFF

        except Exception as e:
            print(f"Error toggling relay: {e}")
            return None
        
    def is_active(self):
        """Check if the session is currently active"""
        return self.is_session_active()
        
    def is_session_active(self):
        if not self.aes_key or not self.aes_iv:
            return False
        
        # Check if session has timed out
        if self.last_activity_time:
            current_time = time.time()
            if current_time - self.last_activity_time > SESSION_TIMEOUT:
                self.end_session()
                return False

    
    def check_session_timeout(self):
        """Check if the session has timed out (1 minute of inactivity)"""
        if self.last_activity_time and (time.time() - self.last_activity_time > 60):
            print("Session has expired due to inactivity.")
            self.end_session()

    def send_command(self, command: bytes):
        encrypted_command, command_hmac = self._encrypt_with_hmac(command)
        self.serial_port.write(encrypted_command + bytes.fromhex(command_hmac))

    def receive_response(self):
        response = self.serial_port.read(32)  # Adjust size
        response_hmac = self.serial_port.read(HMAC_DIGEST_SIZE)
        return self._decrypt_with_hmac_verification(response, response_hmac)

    def end_session(self):
        """End the current session."""
        self.aes_key = None
        self.aes_iv = None
        self.session_id = None
        self.last_activity_time = None
        self._is_active = False
        print("Session ended.")

    def close(self):
        """Close the communication channel and end the session"""
        try:
            self.end_session()
            if hasattr(self, 'serial_port') and self.serial_port:
                self.serial_port.close()
        except Exception as e:
            print(f"Error closing communication: {e}")