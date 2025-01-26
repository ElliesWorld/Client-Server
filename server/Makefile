# Configuration
PORT = /dev/ttyUSB0
SPEED = 115200

# Targets
.PHONY: clean client server default custom

# Clean the project
clean:
	@echo "Cleaning project..."
	@rm -rf .pio server/.vscode
	@rm -rf ../client/__pycache__

# Run the client
client:
	@echo "Running client..."
	@python3 ../client/main.py $(PORT) $(SPEED)

# Build and upload the server
server:
	@echo "Building and uploading server..."
	export PLATFORMIO_BUILD_FLAGS="-DSPEED=$(SPEED)" && \
	pio run -t upload

# Default target (upload server)
default: server

# Build and upload server with custom port and speed
custom:
	@echo "Building server with custom port and speed..."
	export PLATFORMIO_BUILD_FLAGS="-DSPEED=$(SPEED)" && \
	pio run -t upload
