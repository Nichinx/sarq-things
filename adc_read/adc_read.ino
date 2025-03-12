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

// Polynomial coefficients for calibration (example values, update with actual calibration data)
const float coeffs[] = {0.0001, -0.002, 0.03, 0.95};  // Example coefficients for a 3rd degree polynomial


// Correction factors based on empirical data
float correctADC(int rawADC) {
    float voltage = (float)rawADC / ADC_MAX_VALUE * REF_VOLTAGE;
    
    // Apply a second-order polynomial correction based on measured data
    float correctedVoltage = 1.1 * voltage - 0.05 * voltage * voltage;
    
    return correctedVoltage;
}

float calibrateADC(int raw_adc) {
    return 0.000001 * pow(raw_adc, 3) - 0.0001 * pow(raw_adc, 2) + 0.05 * raw_adc + 1.2; 
}

float calculateVIN(uint32_t adcRaw) {
    // Convert raw ADC value to voltage (mV)
    uint32_t adcVoltage_mV = esp_adc_cal_raw_to_voltage(adcRaw, adc_chars);  
    float adcVoltage = adcVoltage_mV / 1000.0;  // Convert mV to V

    // Calculate op-amp gain
    float gain = 1.0 + (R9 / R8);

    // Calculate input voltage VIN
    float VIN = (adcVoltage / gain) * ((R6 + R7) / R6);

    return VIN;
}

// Linear regression calibration function
float calibrateVIN_lin(float vin) {
    // Coefficients from linear regression based on empirical data
    float a = 1.005;  // Slope
    float b = -0.05;  // Intercept

    return a * vin + b;
}

// Polynomial regression calibration function
float calibrateVIN_poly(float vin) {
    // Coefficients from polynomial regression based on empirical data
    float a = 0.0001;  // Coefficient for x^3
    float b = -0.001;  // Coefficient for x^2
    float c = 1.05;    // Coefficient for x
    float d = -0.05;   // Constant term

    return a * pow(vin, 3) + b * pow(vin, 2) + c * vin + d;
}


// **2nd-Order Polynomial Calibration Function**
float calibrateVIN_poly_2(float vin) {
    // Empirically derived calibration coefficients
    float a = 1.002;   // 2nd-order coefficient
    float b = -0.006;  // Linear coefficient
    float c = -0.045;  // Constant offset

    return (a * vin * vin) + (b * vin) + c;
}

// **New Empirical Calibration Function**
float calibrateVIN_emp(float vin) {
    if (vin < 5.0) {
        return vin * 1.01 + 0.05;  // Adjust for lower voltages
    } else if (vin >= 5.0 && vin < 10.0) {
        return vin * 1.005 + 0.02; // Adjust for mid-range voltages
    } else {
        return vin * 1.002 - 0.04; // Adjust for high voltages
    }
}

// Updated Calibration Function
float calibrateVIN_emp_2(float vin) {
    if (vin < 5.0) {
        return vin * 1.008 + 0.03;  // Reduced gain for lower voltages
    } else if (vin >= 5.0 && vin < 10.0) {
        return vin * 1.003 + 0.015; // Reduced gain for mid-range voltages
    } else {
        return vin * 1.0015 - 0.025; // Reduced gain for higher voltages
    }
}

float calibrateVIN_poly_3(float vin) {
    // Apply polynomial calibration
    float calibratedVIN = 0;
    int degree = sizeof(coeffs) / sizeof(coeffs[0]) - 1;
    for (int i = 0; i <= degree; i++) {
        calibratedVIN += coeffs[i] * pow(vin, degree - i);
    }
    return calibratedVIN;
}

float calibrateVIN_lin_2(float vin) {
  // return (vin * 0.993) + 0.010;    // Slightly adjusted slope and offset
  //  return (0.00446 * vin * vin) + (0.9234 * vin) + 0.2264; // QUADRATIC EQUATION
   return (0.00121 * vin * vin * vin) - (0.0298 * vin * vin) + (1.228 * vin) - 0.627;  // RELATIVELY OK!! - CUBIC
  //  return (0.00110 * vin * vin * vin) - (0.0265 * vin * vin) + (1.225 * vin) - 0.605;
  
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);  // Set 12-bit resolution

    //trigger aux 3.3v
    pinMode(AUX_TRIG, OUTPUT);
    delay(500);
    digitalWrite(AUX_TRIG, HIGH);
    delay(500);

     // Allocate memory for ADC characteristics
    adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
    
    // Initialize ADC Calibration
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 'a') {
            int rawADC = analogRead(VMON_PIN);
            // float correctedVoltage = correctADC(rawADC);
            // float calibratedVoltage = calibrateADC(rawADC);
            uint32_t adcCalVoltage = esp_adc_cal_raw_to_voltage(rawADC, adc_chars);  // Corrected voltage in mV
            
            // float VIN = (4.81 * adcCalVoltage) / 1000.0;  // Convert mV to V
            float VIN = calculateVIN(rawADC);
            float calibratedVIN = calibrateVIN_lin_2(VIN);

            Serial.print("Raw ADC: ");
            Serial.print(rawADC);
            // Serial.print(" | Corrected Voltage: ");
            // Serial.print(correctedVoltage, 3);
            // Serial.print(" | Calibrated Voltage: ");
            // Serial.print(calibratedVoltage, 3);
            Serial.print(" | ESP ADC Cal Raw to Voltage: ");
            Serial.print(adcCalVoltage / 1000.0, 3);  // Convert mV to V
            Serial.print(" | Calculated VIN: ");
            Serial.print(VIN, 3);  // Display VIN
            Serial.print(" | Calibrated VIN: ");
            Serial.println(calibratedVIN, 3);  // Display calibrated VIN
        }
    }
    delay(1000);  // Small delay to avoid overwhelming the serial buffer
}
