# Raspberry Pi Pico Arduino Project

An Arduino project targeting the Raspberry Pi Pico using the [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico) framework.

## Prerequisites

- Arduino CLI installed ([install guide](https://arduino.cc/en/Guide/ArduinoCLI))
- ARM GCC toolchain (arm-none-eabi-gcc)
- Raspberry Pi Pico board connected via USB

## Setup

1. Install the Raspberry Pi Pico core:
   ```bash
   arduino-cli core install rp2040:rp2040
   ```

2. List available boards:
   ```bash
   arduino-cli board list
   ```

## Building

```bash
arduino-cli compile -b rp2040:rp2040:rpipico src/main.cpp
```

## Uploading

```bash
arduino-cli upload -b rp2040:rp2040:rpipico -p <SERIAL_PORT> src/main.cpp
```

Replace `<SERIAL_PORT>` with your Pico's serial port (e.g., `/dev/ttyACM0` on Linux/Mac, `COM3` on Windows).

## Project Structure

```
.
├── src/
│   └── main.cpp          # Main sketch file
├── .vscode/
│   ├── c_cpp_properties.json
│   └── settings.json
├── arduino-cli.yaml      # Arduino CLI configuration
├── .devcontainer/        # Development container configuration
└── README.md
```

## C++ Standard

This project is configured for **C++17**.

## Development Environment

- VSCode with C++ IntelliSense configured
- Development container available for consistent environment
