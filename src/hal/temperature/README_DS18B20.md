# DS18B20 Temperature Sensor Setup

This project demonstrates how to use a DS18B20 digital temperature sensor with an Arduino Nano ESP32.

## Hardware Requirements

- Arduino Nano ESP32
- DS18B20 temperature sensor module (3-pin with built-in pull-up resistor)
- Breadboard and jumper wires

## Wiring Diagram

```
Arduino Nano ESP32    DS18B20 Module
------------------    --------------
3.3V              ->  VCC (Power)
GND               ->  GND (Ground)
GPIO 2            ->  DATA (Data)
```

## Pin Connections

| Arduino Nano ESP32 | DS18B20 Module | Description |
|-------------------|----------------|-------------|
| 3.3V              | VCC            | Power supply |
| GND               | GND            | Ground |
| GPIO 2            | DATA           | Data line |

**Note**: This setup is for a 3-pin DS18B20 module that includes a built-in pull-up resistor. If you have a bare DS18B20 sensor, you'll need to add a 4.7kΩ pull-up resistor between the data line and 3.3V.

## Features

- **Automatic device discovery**: The code automatically finds and configures the DS18B20
- **High precision**: Uses 12-bit resolution for accurate readings
- **Error handling**: Detects disconnected sensors and reports errors
- **Multiple units**: Can be easily extended to support multiple DS18B20 sensors
- **Serial output**: Displays temperature in both Celsius and Fahrenheit

## Configuration

### Pin Assignment
Change the `ONE_WIRE_BUS` define in `main.cpp` to use a different GPIO pin:
```cpp
#define ONE_WIRE_BUS 2  // Change this to your preferred pin
```

### Temperature Precision
Adjust the resolution (9-12 bits) by changing:
```cpp
#define TEMPERATURE_PRECISION 12  // 9, 10, 11, or 12 bits
```

### Reading Interval
Modify how often temperature is read:
```cpp
const unsigned long TEMP_READ_INTERVAL = 2000; // 2 seconds
```

## Usage

1. **Upload the code** to your Arduino Nano ESP32
2. **Open the Serial Monitor** at 115200 baud
3. **Check the output** for device discovery and temperature readings

## Expected Output

```
DS18B20 Temperature Sensor Test
Found 1 temperature sensor(s).
Device 0 Address: 28FF1234567890AB
Device 0 Resolution: 12
Setup complete. Starting temperature readings...
Temperature: 23.50°C / 74.30°F
Temperature: 23.56°C / 74.41°F
Temperature: 23.52°C / 74.34°F
```

## Troubleshooting

### No sensors found
- Check wiring connections
- Verify the 4.7kΩ pull-up resistor is connected
- Try a different GPIO pin
- Ensure the DS18B20 is powered with 3.3V

### Invalid readings
- Check for loose connections
- Verify the sensor is not damaged
- Try reducing the wire length between Arduino and sensor

### Communication errors
- Ensure only one device is connected per data line
- Check for electrical interference
- Verify the pull-up resistor value (4.7kΩ recommended)

## Integration with srdriver

Once this is working, you can integrate the DS18B20 into your srdriver firmware by:

1. Adding the libraries to your srdriver `platformio.ini`
2. Creating a temperature sensor class/module
3. Adding temperature monitoring to your main control loop
4. Implementing temperature-based behaviors or safety features

## Library Dependencies

- **OneWire**: Handles the 1-Wire communication protocol
- **DallasTemperature**: Provides high-level interface for DS18B20 sensors 