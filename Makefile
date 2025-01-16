# Configuration 
SPEED = 115200 
PORT = /dev/ttyUSB0 

# Targets 
.PHONY: clean client server 

# Clean Project 
clean: 
	@echo "Cleaning project..."
	@rm -rf server/.pio server/.vscode 
	@rm -rf client/__pycache__ 

# Run Client 
client: 
	@echo "Running client..."
	@python3 client/main.py --port $(PORT) --baud $(SPEED) 

# Build and Upload Server 
server: 
	@echo "Building and uploading server..."
	@cd server && \ 
	export PLATFORMIO_BUILD_FLAGS="-DSPEED=$(SPEED)" && \ 
	pio run -t upload 

# Default upload 
default: server 

# Custom port and speed 
custom: 
	@echo "Building server with custom port and speed..."
	@cd server && \ 
	export PLATFORMIO_BUILD_FLAGS="-DSPEED=$(SPEED)" && \ 
	pio run -t upload 

# Run client 
run_client: 
	@echo "Running client..."
	@python3 client/main.py --port $(PORT) --baud $(SPEED) 

# Clean project 
clean_project: 
	@echo "Cleaning project..."
	@rm -rf server/.pio server/.vscode 
	@rm -rf client/__pycache__ 

# Default upload 
default_upload: server 

# Custom port and speed 
custom_upload: 
	@echo "Building server with custom port and speed..."
	@cd server && \ 
	export PLATFORMIO_BUILD_FLAGS="-DSPEED=$(SPEED)" && \ 
	pio run -t upload 

# Run client 
run_client: 
	@echo "Running client..."
	@python3 client/main.py --port $(PORT) --baud $(SPEED) 

# Clean project 
clean_project: 
	@echo "Cleaning project..."
	@rm -rf server/.pio server/.vscode 
	@rm -rf client/__pycache__ 
