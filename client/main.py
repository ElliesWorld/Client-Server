import sys 
import argparse 
import os, struct
from PyQt6.QtGui import QCursor
from PyQt6.QtCore import ( Qt, QTimer )
from PyQt6.QtWidgets import ( 
    QApplication, QMainWindow, QPushButton,  
    QLabel, QVBoxLayout, QWidget, QTextEdit, QHBoxLayout, QSpacerItem, QSizePolicy
) 
from communication import Communication  # Updated import

class SecureClientUI(QMainWindow): 
    def __init__(self, port: str, baud_rate: int): 
        super().__init__() 
        self.communication = Communication(port, baud_rate) 
        self.initUI() 
  
    def initUI(self): 
        self.setWindowTitle("Client") 
        self.setGeometry(100, 100, 400, 500) 
  
        # Main layout 
        layout = QVBoxLayout() 

        # Horizontal layout for the buttons
        button_layout = QHBoxLayout()
  
        # Session Button 
        self.session_button = QPushButton("Establish Session") 
        self.session_button.clicked.connect(self.toggle_session) 
        button_layout.addWidget(self.session_button) 

                # Temperature Button 
        self.temp_button = QPushButton("Get Temperature") 
        self.temp_button.clicked.connect(self.get_temperature) 
        self.temp_button.setEnabled(False) 
        button_layout.addWidget(self.temp_button) 
  
        # Relay Toggle Button 
        self.relay_button = QPushButton("Toggle Relay") 
        self.relay_button.clicked.connect(self.toggle_relay) 
        self.relay_button.setEnabled(False) 
        button_layout.addWidget(self.relay_button)

        # Fixed space (4 cm)
        spacer = QSpacerItem(80, 0, QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Minimum)
        button_layout.addSpacerItem(spacer)

        # Hyperlink-style "Clear Log"
        self.clear_log_label = QLabel("<a href='#'>Clear</a>")
        self.clear_log_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextBrowserInteraction)
        self.clear_log_label.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.clear_log_label.linkActivated.connect(self.clear_log)
        button_layout.addWidget(self.clear_log_label)

        # Add the horizontal button layout to the main vertical layout
        layout.addLayout(button_layout)
  
        # Log Area 
        self.log_area = QTextEdit() 
        self.log_area.setReadOnly(True) 
        layout.addWidget(self.log_area) 
  
        # Central Widget 
        central_widget = QWidget() 
        central_widget.setLayout(layout) 
        self.setCentralWidget(central_widget) 

        # Setup session timeout timer
        self.session_timeout_timer = QTimer(self)
        self.session_timeout_timer.timeout.connect(self.check_session_timeout)
        self.session_timeout_timer.start(30000)  # Check every 30 seconds

    def check_session_timeout(self):
        if not self.communication.is_session_active():
            self.toggle_session()  # Automatically close the session
  
    def log_message(self, message: str): 
        """Add message to log area""" 
        self.log_area.append(message) 
  
    def toggle_session(self): 
        try: 
            if not self.communication.is_session_active():
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
                self.communication.close() 
                self.session_button.setText("Establish Session") 
                self.temp_button.setEnabled(False) 
                self.relay_button.setEnabled(False) 
                self.log_message("Session Terminated") 
        except Exception as e: 
            self.log_message(f"Session Error: {e}") 

    def check_session_timeout(self):
        if not self.communication.is_session_active():
            self.toggle_session()  # Automatically close the session
  
    def get_temperature(self): 
        try: 
            self.communication.send_command(b"GET_TEMP")  # Send command to get temperature
            response = self.communication.communication_read(4)  # Receive response
            temperature = struct.unpack('f', response)[0]  # Assuming response is a float
            self.log_message(f"Temperature: {temperature}°C") 
        except Exception as e: 
            self.log_message(f"Temperature Error: {e}") 
  
    def toggle_relay(self): 
        try: 
            self.communication.send_command(b"TOGGLE_RELAY")  # Send command to toggle relay
            response = self.communication.communication_read(1)  # Receive response
            relay_state = struct.unpack('B', response)[0]  # Assuming response is a byte
            state_str = "ON" if relay_state else "OFF" 
            self.log_message(f"Relay Toggled: {state_str}") 
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