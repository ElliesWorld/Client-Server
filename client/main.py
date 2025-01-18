import sys
from PyQt6.QtWidgets import (
    QMainWindow, QApplication, QVBoxLayout, QHBoxLayout, 
    QPushButton, QTextEdit, QWidget, QLabel, QSpacerItem, 
    QSizePolicy, QMessageBox
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QCursor

from session import Session

class MainWindow(QMainWindow):
    def __init__(self):      
        super().__init__()
        
        # Initialize session to None
        self.session = None
        
        try:
            # Initialize the session with communication info
            self.communication = "/dev/ttyUSB0:115200"
        except Exception as e:
            self.show_error_dialog("Initialization Error", str(e))
        
        # Initialize the UI
        self.initUI()

    def show_error_dialog(self, title, message):
        """Display an error dialog with the given title and message."""
        error_dialog = QMessageBox()
        error_dialog.setIcon(QMessageBox.Icon.Critical)
        error_dialog.setWindowTitle(title)
        error_dialog.setText(message)
        error_dialog.exec()

    def initUI(self): 
        self.setWindowTitle("ESP32 Client") 
        self.setGeometry(100, 100, 400, 500) 
  
        # Main layout 
        layout = QVBoxLayout() 

        # Horizontal layout for the buttons
        button_layout = QHBoxLayout()
  
        # Establish/Close Session Button
        self.session_button = QPushButton("Establish Session")
        self.session_button.clicked.connect(self.toggle_session)
        button_layout.addWidget(self.session_button)

        # Temperature Button 
        self.temp_button = QPushButton("Get Temperature") 
        self.temp_button.clicked.connect(self.get_temperature) 
        self.temp_button.setEnabled(False)  # Disable until session is established
        button_layout.addWidget(self.temp_button) 
  
        # Relay Toggle Button 
        self.relay_button = QPushButton("Toggle Relay") 
        self.relay_button.clicked.connect(self.toggle_relay) 
        self.relay_button.setEnabled(False)  # Disable until session is established
        button_layout.addWidget(self.relay_button)

        # Fixed space
        spacer = QSpacerItem(80, 0, QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Minimum)
        button_layout.addSpacerItem(spacer)

        # Hyperlink-style "Clear Log"
        self.clear_log_label = QLabel("<a href='#'>Clear Log</a>")
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

    def toggle_session(self):
        """Toggle session establishment/closure."""
        try:
            if self.session is None:
                # Create a new session and try to establish it
                self.session = Session(self.communication)
                if self.session.establish_session():
                    self.session_button.setText("Close Session")
                    self.temp_button.setEnabled(True)
                    self.relay_button.setEnabled(True)
                    self.log_area.append("Session established successfully.")
                else:
                    self.session = None
                    self.log_area.append("Failed to establish session.")
            else:
                # Close the session
                if self.session.close_session():
                    self.session = None
                    self.session_button.setText("Establish Session")
                    self.temp_button.setEnabled(False)
                    self.relay_button.setEnabled(False)
                    self.log_area.append("Session closed successfully.")
                else:
                    self.log_area.append("Failed to close session.")
        except Exception as e:
            self.log_area.append(f"Session toggle error: {str(e)}")

    def get_temperature(self):
        if not self.session:
            self.log_area.append("Communication not initialized")
            return

        try:
            temperature = self.session.get_temperature()
            if temperature != float('nan'):
                self.log_area.append(f"Temperature: {temperature:.2f}°C")
            else:
                self.log_area.append("Could not retrieve temperature")
        except Exception as e:
            self.log_area.append(f"Temperature error: {str(e)}")

    def toggle_relay(self):
        if not self.session:
            self.log_area.append("Communication not initialized")
            return

        try:
            # Toggle relay (alternating between True and False)
            current_state = self.relay_button.property("relay_state")
            new_state = not current_state if current_state is not None else True
            
            relay_result = self.session.toggle_relay(new_state)
            
            self.log_area.append(f"Relay toggled to: {relay_result}")
            self.relay_button.setProperty("relay_state", relay_result)
        except Exception as e:
            self.log_area.append(f"Relay toggle error: {str(e)}")

    def clear_log(self):
        self.log_area.clear()

def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()