#pragma once

#include <Arduino.h>
#include <freertos/LogManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 Configuration
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 12

class DS18B20Component
{
private:
    OneWire oneWire;
    DallasTemperature sensors;
    float _temperatureC = 0.0;
    float _temperatureF = 0.0;
    DeviceAddress _tempSensorAddress;

public:
    DS18B20Component(int pin) : oneWire(pin), sensors(&oneWire)
    {}

    void begin()
    {
        sensors.begin();

        sensors.setResolution(TEMPERATURE_PRECISION);
        LOG_DEBUGF("Found %d temperature sensor(s).", sensors.getDeviceCount());

        if (sensors.getAddress(_tempSensorAddress, 0))
        {
            LOG_DEBUG("Device 0 Address: ");
            for (uint8_t i = 0; i < 8; i++)
            {
                if (_tempSensorAddress[i] < 16) LOG_DEBUG("0");
                LOG_DEBUGF("%02x", _tempSensorAddress[i]);
            }

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
        sensors.requestTemperatures();

        _temperatureC = sensors.getTempC(_tempSensorAddress);
        _temperatureF = sensors.getTempF(_tempSensorAddress);

    }
    float getTemperatureC() { return _temperatureC; }
    float getTemperatureF() { return _temperatureF; }
};

