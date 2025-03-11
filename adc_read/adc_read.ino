#include <Arduino.h>

// Define ADC input pin
#define VMON_PIN 36  // GPIO36 (VP) connected to voltage monitor

// Define ADC characteristics
#define ADC_MAX_VALUE 4095  // 12-bit ADC resolution
#define REF_VOLTAGE 3.3     // Reference voltage (adjust if needed)

// Correction factors based on empirical data
float correctADC(int rawADC) {
    float voltage = (float)rawADC / ADC_MAX_VALUE * REF_VOLTAGE;
    
    // Apply a second-order polynomial correction based on measured data
    float correctedVoltage = 1.1 * voltage - 0.05 * voltage * voltage;
    
    return correctedVoltage;
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);  // Set 12-bit resolution
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 'a') {
            int rawADC = analogRead(VMON_PIN);
            float measuredVoltage = correctADC(rawADC);
            
            Serial.print("Raw ADC: ");
            Serial.print(rawADC);
            Serial.print(" | Corrected Voltage: ");
            Serial.println(measuredVoltage, 3);
        }
    }
    delay(100);  // Small delay to avoid overwhelming the serial buffer
}
