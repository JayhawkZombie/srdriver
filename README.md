# SRDriver LED Controller

Full-stack system for controlling 5050 LEDs, with support for other hardware peripherals.

Notable features are
- SD card support
- BLE connections via the name "SRDriver"
- WebSockets, with support for assigning devices a static IP
- ESP32 Touchscreen controller support
- Multi-tasking with FreeRTOS

## Hardware Requirements

- ESP32-based board
- Any WS21812 LEDs (any with an SPI protocol compatible with FastLED should work)

## Optionally Supported Hardware

- SD Card + adapter
  - Optional but highly recommended for logging and configuration
- SSD1306 Display
  - Also highly recommended to see system at a glance at runtime
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
   <img width="1389" height="1177" alt="Screenshot 2026-01-17 at 10 31 26 AM" src="https://github.com/user-attachments/assets/ffbfd9d1-9fa5-4ac4-b8c2-f7a2740e8c1e" />
5. Access the web interface

## Configuration

There are several things you can configure. The SD card is by default populated with these files
- config
    - settings.json
- data
    - connected_devices.json (for CrowPanel devices)
    - default_effects.json
- logs
    - srdriver.log (default file target for logs)
 

The `settings.json` file can be used to configure:
- The display address for any connected SSD1306 displays
- The number of configured LEDs
- The device name (this is broadcast via web sockets)
- WiFi network information for known networks
    - A static IP address if desired
- The panel configuration

```json
{
    "display": {
        "address": "0x3C"
    },
    "numLEDs": 1024,
    "device": {
        "name": "SRDriver",
    },
    "wifi": {
        "knownNetworks": [
            {
                "ssid": "*******",
                "password": "*******"
            },
            {
                "ssid": "*********",
                "password": "*******"
            }
        ],
        "staticIP": "192.168.1.111"
    },
    "panels": {
        "usePanels": true,
        "panelConfigs": [
            {
                "rows": 16,
                "cols": 16,
                "row0": 0,
                "col0": 0,
                "type": 2,
                "rotIdx": 1,
                "swapTgtRCs": false
            },
            ... others
        ]
    }
}
```

### LED Configuration
Modify `Globals.h` for your LED setup:
```cpp
#define NUM_LEDS 256  // Maximum number of LEDs
#define LED_PIN 8     // LED data pin
```


### Log Filtering
This system makes heavy use of logging, especially as features are developed. You can filter logs by component like this, in main.cpp. Logs for components that appear in this list will appear, they're otherwise discarded.

```cpp
std::vector<String> logFilters = { "Main", "Startup", "WebSocketServer", "WiFiManager", "LVGLDisplay", "DeviceManager", "WebSocketClient"};
LOG_SET_COMPONENT_FILTER(logFilters);
```


## Usage

### Web Interface
> Use the web app here: https://github.com/JayhawkZombie/srdriver-webapp

You can connect either via BLE or if you know the IP address you can connect directly via web sockets

1. Open the web app in a Bluetooth-enabled browser and hit "Connect BLE"
   <img width="2042" height="1166" alt="Screenshot 2026-01-17 at 10 06 16 AM" src="https://github.com/user-attachments/assets/bcba0805-daf4-4152-b945-3f2d892da0c7" />

2. Connect to "SRDriver" device
    <img width="2044" height="1174" alt="Screenshot 2026-01-17 at 10 14 35 AM" src="https://github.com/user-attachments/assets/c5405c0e-a60e-43d7-ab90-29299e680cc2" />
    
3. Control the device!
   <img width="1346" height="1017" alt="Screenshot 2026-01-17 at 10 16 09 AM" src="https://github.com/user-attachments/assets/b4e8d80a-e50c-47aa-9ee3-1003fb7c71bb" />
   You can connect via web sockets to be able to send larger commands, the most complex need web sockets to transmit all the data.
   Support for those over BLE is coming.
   
4. The right panel contains the connection history and communication monitor - all incoming web socket communications are shown here.
   <img width="2034" height="1103" alt="Screenshot 2026-01-17 at 10 27 01 AM" src="https://github.com/user-attachments/assets/0e411267-d843-4dc4-b987-43fd60f9cc92" />
   Click on the blue cable+ button to connect to any device in the device history. It'll time out after 30s if no device is found.
   <img width="391" height="574" alt="Screenshot 2026-01-17 at 10 33 09 AM" src="https://github.com/user-attachments/assets/91922c5e-e113-4cff-9d6d-e6fa00891881" />

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

There is none lol, sorry, but right now this project is just moving forward for feature development.

## License

[MIT License]
Copyright 2026 Kurt Slagle

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
