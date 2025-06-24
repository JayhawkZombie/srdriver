# SRDriver LED Controller

An LED controller with BLE connectivity, featuring some authentication and security measures.

## Features

- **LED Control**: Control brightness, patterns, and colors for multiple LED arrays
- **BLE Connectivity**: Wireless control via Bluetooth Low Energy
- **Authentication System**: PIN-based authentication to prevent unauthorized access
- **Device Pairing**: Secure pairing mode for trusted devices
- **Visual Feedback**: LED indicators for authentication status and pairing mode

## Security Implementation

### BLE Security Architecture

The SRDriver implements a two-service BLE architecture for some security:

1. **Auth Service** (`a1862b70-e0ce-4b1b-9734-d7629eb8d710`)
   - Always advertised and discoverable
   - Contains only the authentication characteristic
   - Allows PIN-based authentication

2. **Control Service** (`b1862b70-e0ce-4b1b-9734-d7629eb8d711`)
   - Only added to BLE stack after successful authentication
   - Contains all LED control characteristics
   - Not discoverable until authentication is complete

### Authentication Flow

1. **Initial Connection**: Device connects to auth service only
2. **PIN Authentication**: Client sends PIN via auth characteristic
3. **Service Addition**: Upon successful auth, control service is dynamically added
4. **Control Access**: Client can now access LED control characteristics

### Security Limitations

**Important**: While this implementation provides some security improvements, it cannot completely prevent direct BLE access from sophisticated tools like LightBlue. Here's why:

#### What This Implementation Prevents:
- ✅ Unauthorized access through the web app
- ✅ Casual BLE scanning and control
- ✅ Most generic BLE apps from controlling the device
- ✅ Automatic device discovery of control features

#### What It Cannot Prevent:
- ❌ Direct BLE characteristic access by advanced tools (LightBlue, nRF Connect, etc.)
- ❌ BLE packet sniffing and replay attacks
- ❌ Man-in-the-middle attacks on the BLE connection

#### Why This Limitation Exists:
1. **BLE Protocol Design**: BLE is designed for easy discovery and connection
2. **ArduinoBLE Library**: Limited security features compared to native BLE stacks
3. **Hardware Constraints**: Teensy/ESP32 BLE implementation limitations

### Recommended Security Practices

1. **Change Default PIN**: Modify `AUTH_PIN` in `src/main.cpp`
2. **Use Pairing Mode**: Long-press secondary button to enter secure pairing
3. **Monitor Connections**: Watch LED indicators for authentication status
4. **Physical Security**: Keep device in controlled environment
5. **Regular PIN Changes**: Update authentication PIN periodically

## Hardware Requirements

- Teensy 4.1 or ESP32-based board
- Any Adafruit NeoPixel LEDs
- Push buttons for control
- Potentiometers for analog input

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
3. Enter PIN (default: "1234")
4. Control LEDs through the interface

### Physical Controls
- **Primary Button**: Change LED patterns
- **Secondary Button**: Toggle color control mode
- **Secondary Button (Long Press)**: Enter pairing mode
- **Potentiometers**: Control brightness, speed, and colors

### Pairing Mode
1. Long-press secondary button until LEDs flash yellow
2. Connect from web app within 30 seconds
3. Enter any PIN to authorize the device
4. Device will remember authorized devices

## LED Indicators

- **Red**: Not authenticated
- **Yellow (Blinking)**: Pairing mode active
- **Normal Colors**: Authenticated and operational

## Configuration

### Changing the PIN
Edit `AUTH_PIN` in `src/main.cpp`:
```cpp
#define AUTH_PIN "1234"  // Change to your preferred PIN
```

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
- Verify PIN is correct
- Try pairing mode for new devices

### LED Problems
- Check power supply voltage and current
- Verify LED data connections
- Ensure correct LED count in configuration

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

This implementation provides **application-level security** rather than **protocol-level security**. While it significantly reduces unauthorized access, it cannot prevent all possible BLE attacks. For maximum security:

1. Use in controlled environments
2. Implement additional physical security measures
3. Consider network-level security if connecting to external systems
4. Regularly update authentication credentials

## License

[Add your license information here]
