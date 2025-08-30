# SRDriver LED Controller

A full-stack system to support controlling of LEDs, response to input (such as audio, temperature, and more in the future),
featuring BLE device controls, BLE SD card viewing (no edits yet), SD card support, using FreeRTOS.

## Features

- **LED Control**: Control brightness, patterns, colors, etc for multiple LED arrays
- **BLE Connectivity**: Wireless control via Bluetooth Low Energy

## Hardware Requirements

- ESP32-based board
- Any Adafruit NeoPixel LEDs
  - Really, any WS21812 LED with a single pin SPI protocol compatible with FastLED should work
- SD Card + adapter
  - Should be optional, currently is required, or device will crash on boot

## Optionally Supported Hardware

- SSD1306 Display
- ACS712 Voltage + Current Sensors
- MAX4466 Microphone
- Momentary push-buttons
- Linear potentiometers (generally 10k ohm)
- DS18B20 Temperature Sensor

## Installation

1. Clone the repository
2. Install PlatformIO
3. Connect your hardware
4. Upload the firmware
5. Access the web interface

## Usage

### Web Interface
1. Open the web app in a Bluetooth-enabled browser
2. Connect to "SRDriver" device (may have to hit Cancel after the first prompt to allow for more flexible device discovery)
3. Control LEDs through the interface
## Configuration


### LED Configuration
Modify `Globals.h` for your LED setup:
```cpp
#define NUM_LEDS 256  // Total number of LEDs
#define LED_PIN 8     // LED data pin
```

## Troubleshooting

### Connection Issues
- Ensure device is powered and in range
- Check browser Bluetooth support
- Ensure no other devices are connected (power cycle if you need to kick other devices off a connection)

### LED Problems
- Check power supply voltage and current
- Verify LED data connections
- Ensure correct LED count in configuration
  - You can configure for more than there really are, but do not configure for less and the remaining ones won't light up

## Development

### Building Firmware
```bash
pio run -t upload
```

### Web App Development
```bash
cd srdriver-webapp
npm install
npm start
```

## Security Considerations

There is none lol

## License

[Add your license information here]
MIT, placeholder though because I don't have the fully copy, but this shit is open-source lol
