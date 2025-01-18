import struct
from communication import Communication

class Session:
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
    
    def __init__(self, cominfo: str):
        self.__SESSION_ID = bytes([0] * 8)
        self.__communication = Communication(cominfo)
        if not self.__communication.connect():
            raise Exception("Failed to connect ...")

    def establish_session(self) -> bool:
        """
        Establish a session with the server.
        
        Returns:
            bool: True if session is established, False otherwise.
        """
        try:
            # Send establish session command
            if not self.__communication.send(bytes([0])):
                raise ConnectionError("Failed to send establish session command")
            
            # Receive response (9 bytes: 1 byte status + 8 bytes session ID)
            response = self.__communication.receive(9)
            if len(response) == 9 and response[0] == self.STATUS_OKAY:
                self.__SESSION_ID = response[1:9]
                return True
            else:
                return False
        
        except Exception as e:
            print(f"Session establishment error: {e}")
            return False

    def get_temperature(self) -> float:
        """
        Get temperature from server
        
        Returns:
            float: Temperature value
        """
        try:
            buffer = bytes([self.__TEMPERATURE]) + self.__SESSION_ID
            # Send temperature command
            if not self.__communication.send(buffer):
                raise ConnectionError("Failed to send temperature command")
            
            # Receive response (5 bytes: 1 byte status + 4 bytes temperature)
            response = self.__communication.receive(5)

            if len(response) == 5 and response[0] == self.STATUS_OKAY:
                value = int.from_bytes(response[1:5], 'little')
                temperature = struct.unpack('>f', value.to_bytes(4, 'big'))[0]
                return temperature
            else:
                print(f"Invalid temperature response. Length: {len(response)}, First byte: {response[0] if response else 'No response'}")
                return float('nan')
        
        except Exception as e:
            print(f"Temperature retrieval error: {e}")
            return float('nan')

    def toggle_relay(self) -> bool:
        """
        Toggle relay on server
        
        Args:
            state (bool): Desired relay state
        
        Returns:
            bool: Actual relay state
        """
        try:
            buffer = bytes([self.__TOGGLE_RELAY]) + self.__SESSION_ID
            if not self.__communication.send(buffer):
                raise ConnectionError("Failed to send relay toggle command")
            
            # Receive response (2 bytes: 1 byte status + 1 byte relay state)
            response = self.__communication.receive(2)
            if len(response) == 2 and response[0] == self.STATUS_OKAY:
                return bool(response[1])
            else:
                print(f"Failed to toggle relay. Response: {response}")
                return False
        
        except Exception as e:
            print(f"Relay toggle error: {e}")
            return False

    def close_session(self) -> bool:
        """
        Close the session with the server.
        
        Returns:
            bool: True if session is closed, False otherwise.
        """
        try:
            buffer = bytes([self.__CLOSE]) + self.__SESSION_ID
            # Send close session command
            if not self.__communication.send(buffer):
                raise ConnectionError("Failed to send close session command")
            
            self.__SESSION_ID = bytes([0] * 8)
            # Receive response (1 byte)
            response = self.__communication.receive(1)
            if len(response) == 1 and response[0] == self.STATUS_OKAY:
                return True
            else:
                return False
        
        except Exception as e:
            print(f"Session closure error: {e}")
            return False
        
    def __bool__(self) -> bool:
        """Check if the session is active."""
        return (0 != int.from_bytes(self.__SESSION_ID, 'little'))