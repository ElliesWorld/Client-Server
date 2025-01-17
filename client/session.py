import struct
from communication import Communication

class Session:
    # Command types
    __TEMPERATURE = 1
    __TOGGLE_RELAY = 2
    
    def __init__(self, communication):
        self.__communication = communication

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