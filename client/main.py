import sys 
import argparse 
from PyQt6.QtWidgets import ( 
    QApplication, QMainWindow, QPushButton,  
    QLabel, QVBoxLayout, QWidget, QTextEdit 
) 
from communication import SecureCommunication 
  
class SecureClientUI(QMainWindow): 
    def __init__(self, port: str, baud_rate: int): 
        super().__init__() 
        self.communication = SecureCommunication(port, baud_rate) 
        self.initUI() 
  
    def initUI(self): 
        self.setWindowTitle("Client") 
        self.setGeometry(100, 100, 400, 500) 
  
        # Main layout 
        layout = QVBoxLayout() 
  
        # Session Button 
        self.session_button = QPushButton("Establish Session") 
        self.session_button.clicked.connect(self.toggle_session) 
        layout.addWidget(self.session_button) 

        # Temperature Button 
        self.temp_button = QPushButton("Get Temperature") 
        self.temp_button.clicked.connect(self.get_temperature) 
        self.temp_button.setEnabled(False) 
        layout.addWidget(self.temp_button) 
  
        # Relay Toggle Button 
        self.relay_button = QPushButton("Toggle Relay") 
        self.relay_button.clicked.connect(self.toggle_relay) 
        self.relay_button.setEnabled(False) 
        layout.addWidget(self.relay_button) 
  
        # Log Area 
        self.log_area = QTextEdit() 
        self.log_area.setReadOnly(True) 
        layout.addWidget(self.log_area) 
  
        # Clear Log Button 
        clear_log_button = QPushButton("Clear Log") 
        clear_log_button.clicked.connect(self.clear_log) 
        layout.addWidget(clear_log_button) 
  
        # Central Widget 
        central_widget = QWidget() 
        central_widget.setLayout(layout) 
        self.setCentralWidget(central_widget) 
  
    def log_message(self, message: str): 
        """Add message to log area""" 
        self.log_area.append(message) 
  
    def toggle_session(self): 
        try: 
            if not self.communication.session.is_session_valid(): 
                # Establish Session 
                if self.communication.establish_session(): 
                    self.session_button.setText("Close Session") 
                    self.temp_button.setEnabled(True) 
                    self.relay_button.setEnabled(True) 
                    self.log_message("Session Established") 
                else: 
                    self.log_message("Session Establishment Failed") 
            else: 
                # Close Session 
                self.communication.session.end_session() 
                self.session_button.setText("Establish Session") 
                self.temp_button.setEnabled(False) 
                self.relay_button.setEnabled(False) 
                self.log_message("Session Terminated") 
        except Exception as e: 
            self.log_message(f"Session Error: {e}") 
  
    def get_temperature(self): 
        try: 
            temperature = self.communication.get_temperature() 
            if temperature is not None: 
                self.log_message(f"Temperature: {temperature}°C") 
            else: 
                self.log_message("Failed to get temperature") 
        except Exception as e: 
            self.log_message(f"Temperature Error: {e}") 
  
    def toggle_relay(self): 

        try: 
            relay_state = self.communication.toggle_relay() 
            if relay_state is not None: 
                state_str = "ON" if relay_state else "OFF" 
                self.log_message(f"Relay Toggled: {state_str}") 
            else: 
                self.log_message("Failed to toggle relay") 
        except Exception as e: 
            self.log_message(f"Relay Toggle Error: {e}") 
  
    def clear_log(self): 
        """Clear log area""" 
        self.log_area.clear() 
  
    def closeEvent(self, event): 
        """Handle application close""" 
        try: 
            # Ensure session is closed and connection is terminated 
            self.communication.close() 
        except Exception as e: 
            print(f"Cleanup error: {e}") 
        event.accept() 
  
def main(): 
    # Argument parsing 
    parser = argparse.ArgumentParser(description="Secure Serial Communication Client") 
    parser.add_argument( 
        '--port',  
        type=str,  
        required=True,  
        help='Serial port (e.g., /dev/ttyUSB0 or COM3)' 
    ) 
    parser.add_argument( 
        '--baud',  
        type=int,  
        default=115200,  
        help='Baud rate (default: 115200)' 
    ) 
  
    # Parse arguments 
    args = parser.parse_args() 
  
    # Create application 
    app = QApplication(sys.argv) 

    try: 
        # Create and show main window 
        client_ui = SecureClientUI(args.port, args.baud) 
        client_ui.show() 

        # Run application 
        sys.exit(app.exec()) 

    except Exception as e: 
        print(f"Application initialization error: {e}") 
        sys.exit(1) 

if __name__ == "__main__": 
    main() 