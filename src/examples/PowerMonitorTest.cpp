// /**
//  * Power Monitoring Test
//  * 
//  * Simple test to verify ACS712 current and voltage sensors are working
//  * Run this to validate your sensor wiring and calibration
//  */

// #include <Arduino.h>
// #include "../hal/power/ACS712CurrentSensor.h"
// #include "../hal/power/ACS712VoltageSensor.h"

// // Test configuration for 5V supply monitoring
// ACS712CurrentSensor currentSensor(A2, 5.0, 3.3, ACS712_30A);
// ACS712VoltageSensor voltageSensor(A3, 5.0, 3.3, 1.5);

// void setup() {
//     Serial.begin(115200);
//     while (!Serial) {
//         delay(100);
//     }
    
//     Serial.println("=== ACS712 Power Monitoring Test ===");
//     Serial.println("Wiring check:");
//     Serial.println("- Current sensor big terminals: 5V supply through IP+ to IP-");
//     Serial.println("- Voltage sensor terminals: parallel across load");
//     Serial.println("- Current sensor pins: VCC->5V, GND->GND, OUT->A2");
//     Serial.println("- Voltage sensor pins: VCC->5V, GND->GND, OUT->A3");
//     Serial.println();
    
//     // Initialize sensors
//     currentSensor.begin();
//     voltageSensor.begin();
    
//     // Print configuration
//     currentSensor.printConfiguration();
//     voltageSensor.printConfiguration();
    
//     Serial.println("\n=== Starting continuous monitoring ===");
//     Serial.println("Expected readings for ESP32 on 5V supply:");
//     Serial.println("- Current: 300-800mA (depending on activity)");
//     Serial.println("- Voltage: 4.8-5.2V");
//     Serial.println("- Power: 1.5-4W");
//     Serial.println();
// }

// void loop() {
//     // Read sensors
//     float current_mA = currentSensor.readCurrentDCFiltered_mA();
//     float voltage_V = voltageSensor.readVoltageDCFiltered_V();
//     float power_W = (current_mA / 1000.0) * voltage_V;
    
//     // Display readings
//     Serial.printf("Power: %.0fmA @ %.2fV = %.2fW", current_mA, voltage_V, power_W);
    
//     // Add status indicators
//     if (current_mA < 100) {
//         Serial.print(" [VERY LOW - check wiring]");
//     } else if (current_mA > 1000) {
//         Serial.print(" [HIGH CURRENT]");
//     }
    
//     if (voltage_V < 4.5) {
//         Serial.print(" [LOW VOLTAGE]");
//     } else if (voltage_V > 5.5) {
//         Serial.print(" [HIGH VOLTAGE]");
//     }
    
//     Serial.println();
    
//     // Print raw diagnostics occasionally
//     static uint32_t lastDiagnostic = 0;
//     if (millis() - lastDiagnostic > 10000) {  // Every 10 seconds
//         Serial.println("\n=== Raw Sensor Diagnostics ===");
//         currentSensor.printRawDiagnostics();
//         voltageSensor.printRawDiagnostics();
//         Serial.println("====================================\n");
//         lastDiagnostic = millis();
//     }
    
//     delay(1000);
// } 