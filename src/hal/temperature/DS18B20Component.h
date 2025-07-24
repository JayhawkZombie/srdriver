#pragma once

#include <Arduino.h>
#include <freertos/LogManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 Configuration
#define ONE_WIRE_BUS 2  // GPIO pin where DS18B20 is connected
#define TEMPERATURE_PRECISION 12  // 9-12 bit resolution

class DS18B20Component
{
private:
    OneWire oneWire;
    DallasTemperature sensors;
    // Variables to store temperature data
    float _temperatureC = 0.0;
    float _temperatureF = 0.0;
    // Device address for the DS18B20 (will be discovered automatically)
    DeviceAddress _tempSensorAddress;

public:
    DS18B20Component(int pin) : oneWire(pin), sensors(&oneWire)
    {}

    void begin()
    {
        // Initialize the temperature sensor
        sensors.begin();

        // Set the resolution
        sensors.setResolution(TEMPERATURE_PRECISION);
        // Discover and print device information
        LOG_DEBUGF("Found %d temperature sensor(s).", sensors.getDeviceCount());

        // Get the first device address
        if (sensors.getAddress(_tempSensorAddress, 0))
        {
            LOG_DEBUG("Device 0 Address: ");
            for (uint8_t i = 0; i < 8; i++)
            {
                if (_tempSensorAddress[i] < 16) LOG_DEBUG("0");
                LOG_DEBUGF("%02x", _tempSensorAddress[i]);
            }

            // Set the resolution for this specific device
            sensors.setResolution(_tempSensorAddress, TEMPERATURE_PRECISION);

            LOG_DEBUGF("Device 0 Resolution: %d", sensors.getResolution(_tempSensorAddress));
        }
        else
        {
            LOG_DEBUG("Unable to find address for Device 0");
        }

        LOG_DEBUG("Setup complete. Starting temperature readings...");
    }
    void update()
    {
        // Request temperature readings from all sensors
        sensors.requestTemperatures();

        // Read temperature from the first sensor
        _temperatureC = sensors.getTempC(_tempSensorAddress);
        _temperatureF = sensors.getTempF(_tempSensorAddress);

    }
    float getTemperatureC() { return _temperatureC; }
    float getTemperatureF() { return _temperatureF; }
};

