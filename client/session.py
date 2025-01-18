import struct
from communication import Communication

class Session:
    # Command types
    __ESTABLISH_SESSION = 0
    __TEMPERATURE = 1
    __TOGGLE_RELAY = 2
    __CLOSE_SESSION = 3

    STATUS_OKAY = 0
    STATUS_ERROR = 1
    
    def __init__(self, cominfo: str):
        self.__SESSION_ID = 0
        self.__communication = Communication(cominfo)
        self.__is_active = False

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
            
            # Receive response (9 bytes: 1 byte status + 8 bytes session ID)
            response = self.__communication.receive(9)
            if len(response) == 9 and response[0] == self.STATUS_OKAY:
                self.__SESSION_ID = response[1:9]
                self.__is_active = True
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
            # Send temperature command
            if not self.__communication.send(bytes([self.__TEMPERATURE])):
                raise ConnectionError("Failed to send temperature command")
            
            # Receive response (5 bytes: 1 byte status + 4 bytes temperature)
            response = self.__communication.receive(5)
            
            if len(response) == 5 and response[0] == self.STATUS_OKAY:
                # Unpack the float
                # Use little-endian to match Arduino's memory layout
                temperature = struct.unpack('<f', response[1:5])[0]
                
                # Optional: Add sanity check
                if -100 <= temperature <= 100:
                    return temperature
                else:
                    print(f"Unexpected temperature value: {temperature}")
                    return float('nan')
            else:
                print(f"Invalid temperature response. Length: {len(response)}, First byte: {response[0] if response else 'No response'}")
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
            command = bytes([self.__TOGGLE_RELAY, int(state)])
            if not self.__communication.send(command):
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
            # Send close session command
            if not self.__communication.send(bytes([self.__CLOSE_SESSION])):
                raise ConnectionError("Failed to send close session command")
            # Receive response (1 byte)
            response = self.__communication.receive(1)
            if response and response[0] == self.STATUS_OKAY:
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