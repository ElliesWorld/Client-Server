import struct, serial
from communication import Communication
from mbedtls import pk, hmac, hashlib, cipher

class Session:
    __RSA_SIZE = 256
    __EXPONENT = 65537
    __SECRET_KEY = b"Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+"

    # Command types
    __TEMPERATURE = 1
    __TOGGLE_RELAY = 2
    __CLOSE = 3

    STATUS_OKAY = 0
    STATUS_ERROR = 1
    STATUS_EXPIRED = 2
    STATUS_HASH_ERROR = 3
    STATUS_BAD_REQUEST = 4
    STATUS_INVALID_SESSION = 5
    STATUS_COMMUNICATION_ERROR = 6
    
    def __init__(self, cominfo: str):
        self.__SESSION_ID = bytes([0] * 8)
        self.__communication = Communication(cominfo)
        if not self.__communication.connect():
            raise Exception("Failed to connect ...")
        
        self.__hmac = hashlib.sha256()
        self.__hmac.update(Session.__SECRET_KEY)
        self.__hmac = self.__hmac.digest()
        self.__hmac = hmac.new(self.__hmac, digestmod="SHA256")

        self.__client_rsa = pk.RSA()
        self.__client_rsa.generate(self.__RSA_SIZE * 8, Session.__EXPONENT)

        if not self.__send(self.__client_rsa.export_public_key()):
            raise Exception("Failed to send public key ...")
        
        buffer = self.__receive(2 * Session.__RSA_SIZE)
        if 0 == len(buffer):
            raise Exception("Failed to receive the server public key ...")
        
        # RSA private key decryption
        self.__server_rsa = self.__client_rsa.decrypt(buffer[0:Session.__RSA_SIZE])
        self.__server_rsa += self.__client_rsa.decrypt(buffer[Session.__RSA_SIZE:2*Session.__RSA_SIZE])
        self.__server_rsa = pk.RSA().from_DER(self.__server_rsa)

        del self.__client_rsa
        self.__client_rsa = pk.RSA()
        self.__client_rsa.generate(Session.__RSA_SIZE * 8, Session.__EXPONENT)

        buffer = self.__client_rsa.export_public_key() + self.__client_rsa.sign(Session.__SECRET_KEY, "SHA256")
        buffer = self.__server_rsa.encrypt(buffer[0:184]) + self.__server_rsa.encrypt(buffer[184:368]) + self.__server_rsa.encrypt(buffer[368:550])

        if not self.__send(buffer):
            raise Exception("Failed to send the client new public key ...")
        
        buffer = self.__receive(Session.__RSA_SIZE)
        if 0 == len(buffer):
            raise Exception("Failed to receive the exchange status ...")
        
        if b"DONE" != self.__client_rsa.decrypt(buffer):
            raise Exception("Failed to exchange the public keys ...")

    def __send(self, buf: bytes) -> bool:
        self.__hmac.update(buf)
        buf +=  self.__hmac.digest()
        return self.__communication.send(buf)

    def __receive(self, length: int) -> bytes:
        buffer = self.__communication.receive(length + self.__hmac.digest_size)
        self.__hmac.update(buffer[0:length])
        if buffer[length:length + self.__hmac.digest_size] != self.__hmac.digest():
            buffer = b''
        else:
            buffer = buffer[0:length]
        return buffer

    def establish_session(self) -> bool:
        """
        Establish a session with the server.
        
        Returns:
            bool: True if session is established, False otherwise.
        """
        self.__SESSION_ID = bytes([0] * 8)
        buffer = self.__client_rsa.sign(Session.__SECRET_KEY, "SHA256")
        buffer = self.__server_rsa.encrypt(buffer[0:Session.__RSA_SIZE//2]) + self.__server_rsa.encrypt(buffer[Session.__RSA_SIZE//2:Session.__RSA_SIZE])
        if self.__send(buffer):
            buffer = self.__receive(Session.__RSA_SIZE)
            if 0 == len(buffer):
                raise Exception("Failed to receive the session info ...")
            buffer = self.__client_rsa.decrypt(buffer)
            self.__SESSION_ID = buffer[0:8]
            self.__AES = cipher.AES.new(buffer[8:40], cipher.MODE_CBC, buffer[40:56])
            return True
        else:
            return False

    def __request(self, req: int, res: bytearray) -> int:
        status = Session.STATUS_INVALID_SESSION

        if 0 != int.from_bytes(self.__SESSION_ID, 'little'):
            buffer = bytes([req]) + self.__SESSION_ID
            plen = cipher.AES.block_size - (len(buffer) % cipher.AES.block_size)
            buffer = self.__AES.encrypt(buffer + bytes([len(buffer)] * plen))
            if self.__send(buffer):
                buffer = self.__receive(cipher.AES.block_size)
                if 0 == len(buffer):
                    status = Session.STATUS_COMMUNICATION_ERROR
                else:
                    buffer = self.__AES.decrypt(buffer)
                    if len(buffer) > 1:
                        res[:] = buffer[1:]
                    status = buffer[0]
            else:
                status = Session.STATUS_COMMUNICATION_ERROR

        if status == Session.STATUS_COMMUNICATION_ERROR or status == Session.STATUS_EXPIRED:
            self.__SESSION_ID = bytes([0] * 8)

        return status
    
    def get_temperature(self) -> float:
        """
        Get temperature from server
        
        Returns:
            float: Temperature value
        """
        buffer = bytearray(4)
        status = self.__request(self.__TEMPERATURE, buffer)
        if status != Session.STATUS_OKAY:
            raise Exception(status)

        temperature = int.from_bytes(buffer, 'little')
        temperature = struct.unpack('>f', temperature.to_bytes(4, 'big'))[0]
        return temperature

    def toggle_relay(self) -> bool:
        """
        Toggle relay on server
        
        Args:
            state (bool): Desired relay state
        
        Returns:
            bool: Actual relay state
        """
        buffer = bytearray(1)
        status = self.__request(self.__TOGGLE_RELAY, buffer)
        if status != Session.STATUS_OKAY:
            raise Exception(status)

        return (buffer[0] != 0)


    def close_session(self) -> bool:
        """
        Close the session with the server.
        
        Returns:
            bool: True if session is closed, False otherwise.
        """
        status = (Session.STATUS_OKAY == self.__request(self.__CLOSE, bytearray()))
        self.__SESSION_ID = bytes([0] * 8)
        return status
        
    def __bool__(self) -> bool:
        """Check if the session is active."""
        return (0 != int.from_bytes(self.__SESSION_ID, 'little'))