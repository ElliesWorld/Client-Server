import struct
from communication import Communication

class Session:
    # Command types
    __ESTABLISH_SESSION = 0  # New command for establishing session
    __CLOSE_SESSION = 3       # New command for closing session
    __TEMPERATURE = 1
    __TOGGLE_RELAY = 2
    
    def __init__(self, communication):
        self.__communication = communication
        self.__is_active = False  # Track session state

    def establish_session(self) -> bool:
        """
        Establish a session with the server.
        
        Returns:
            bool: True if session is established, False otherwise.
        """
        try:
            # Send establish session command
            if not self.__communication.send(bytes([self.__ESTABLISH_SESSION])):
                raise ConnectionError("Failed to send establish session command")
            
            # Receive response (1 byte)
            response = self.__communication.receive(1)
            if response and response[0] == 0:  # Assuming 0 means success
                self.__is_active = True
                return True
            else:
                return False
        
        except Exception as e:
            print(f"Session establishment error: {e}")
            return False

    def close_session(self) -> bool:
        """
        Close the session with the server.
        
        Returns:
            bool: True if session is closed, False otherwise.
        """
        try:
            # Send close session command
            if not self.__communication.send(bytes([self.__CLOSE_SESSION])):
                raise ConnectionError("Failed to send close session command")
            
            # Receive response (1 byte)
            response = self.__communication.receive(1)
            if response and response[0] == 0:  # Assuming 0 means success
                self.__is_active = False
                return True
            else:
                return False
        
        except Exception as e:
            print(f"Session closure error: {e}")
            return False

    def is_active(self) -> bool:
        """Check if the session is active."""
        return self.__is_active

    def get_temperature(self) -> float:
        """
        Get temperature from server
        
        Returns:
            float: Temperature value
        """
        try:
            # Send temperature command
            if not self.__communication.send(bytes([1])):
                raise ConnectionError("Failed to send temperature command")
            
            # Receive 4 bytes (float)
            response = self.__communication.receive(4)
            
            if len(response) == 4:
                value = int.from_bytes(response, 'little')
                temperature = struct.unpack('>f', value.to_bytes(4, 'big'))[0]
                return temperature
            else:
                print(f"Unexpected response length: {len(response)}")
                return float('nan')
        
        except Exception as e:
            print(f"Temperature retrieval error: {e}")
            return float('nan')

    def toggle_relay(self, state: bool) -> bool:
        """
        Toggle relay on server
        
        Args:
            state (bool): Desired relay state
        
        Returns:
            bool: Actual relay state
        """
        try:
            # Send toggle relay command and state
            command = bytes([2, int(state)])
            if not self.__communication.send(command):
                raise ConnectionError("Failed to send relay toggle command")
            
            # Receive relay state
            response = self.__communication.receive(1)
            
            if response:
                return bool(response[0])
            else:
                print("No response received for relay toggle")
                return False
        
        except Exception as e:
            print(f"Relay toggle error: {e}")
            return False