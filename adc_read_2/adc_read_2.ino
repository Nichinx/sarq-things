#include <Arduino.h>
#include <esp_adc_cal.h>

// Define ADC input pin
#define VMON_PIN 39  // GPIO39 (VP) connected to voltage monitor
#define AUX_TRIG 26

// Define ADC characteristics
#define ADC_MAX_VALUE 4095  // 12-bit ADC resolution
#define REF_VOLTAGE 3.3     // Reference voltage (adjust if needed

// Measured resistor values (update with actual measurements)
#define R6 100000.0  // 100kΩ
#define R7 10000000.0  // 10MΩ
#define R8 10000.0  // 10kΩ
#define R9 200000.0  // 200kΩ

esp_adc_cal_characteristics_t *adc_chars;
#define DEFAULT_VREF 1100  // Default VREF in mV

float calculateVIN(uint32_t adcRaw) {
    uint32_t adcVoltage_mV = esp_adc_cal_raw_to_voltage(adcRaw, adc_chars);  // Convert raw ADC value to voltage (mV)
    float adcVoltage = adcVoltage_mV / 1000.0;  // Convert mV to V

    float gain = 1.0 + (R9 / R8);   // Calculate op-amp gain  
    float VIN = (adcVoltage / gain) * ((R6 + R7) / R6);  // Calculate input voltage VIN

    return VIN;
}

float calibrateVIN(float vin) {
   return (0.00121 * vin * vin * vin) - (0.0298 * vin * vin) + (1.228 * vin) - 0.627; 
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12); 

    //trigger aux 3.3v
    pinMode(AUX_TRIG, OUTPUT);
    delay(500);
    digitalWrite(AUX_TRIG, HIGH);
    delay(500);

    adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));     // Allocate memory for ADC characteristics
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);    // Initialize ADC Calibration
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 'a') {
            int rawADC = analogRead(VMON_PIN);
            uint32_t adcCalVoltage = esp_adc_cal_raw_to_voltage(rawADC, adc_chars);  // Corrected voltage in mV

            float VIN = calculateVIN(rawADC);
            float calibratedVIN = calibrateVIN(VIN);

            Serial.print("Raw ADC: ");
            Serial.print(rawADC);
            Serial.print(" | ESP ADC Cal Raw to Voltage: ");
            Serial.print(adcCalVoltage / 1000.0, 3);  // Convert mV to V
            Serial.print(" | Calculated VIN: ");
            Serial.print(VIN, 3);  // Display VIN
            Serial.print(" | Calibrated VIN: ");
            Serial.println(calibratedVIN, 3);  // Display calibrated VIN
        }
    }
    delay(1000);  
}
