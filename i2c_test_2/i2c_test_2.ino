#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

SFE_UBLOX_GNSS myGNSS;

#define SDA_PIN 21
#define SCL_PIN 22
#define AUX_TRIG 26
#define BAUDRATE 115200

void scanI2CDevices() {
    Serial.println("Scanning I2C bus...");
    bool deviceFound = false;

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device found at I2C address: 0x");
            Serial.println(address, HEX);
            deviceFound = true;

            // // Identify known devices
            // if (address == 0x42) Serial.println(" → Ublox GNSS detected!");
            // if (address == 0x68) Serial.println(" → Possible IMU (MPU6050 or similar)");
        }
    }

    if (deviceFound) {
        Serial.println("External I2C is working.");
    } else {
        Serial.println("No I2C devices found!");
    }
}

bool init_ublox() {
    Serial.println("Initializing UBlox GNSS...");

    for (int attempts = 0; attempts < 3; attempts++) {  // 3 retries
        if (myGNSS.begin(Wire)) {
            Serial.println("UBlox GNSS initialized.");
            
            // Configure GNSS module settings
            myGNSS.setI2COutput(COM_TYPE_UBX); // Output only UBX messages
            myGNSS.setNavigationFrequency(5);  // Set GNSS update rate to 5Hz
            myGNSS.setHighPrecisionMode(true);  
            myGNSS.powerSaveMode(false);       // Disable power-saving for high update rates
            Serial.println("UBlox GNSS configured successfully.");
            return true;
        }

        Serial.println(F("UBlox GNSS not detected at default I2C address. Retrying..."));
        delay(1000);
    }

    Serial.println("Failed to initialize UBlox GNSS.");
    return false;
}

void setup() {
    Serial.begin(BAUDRATE);

    // Trigger AUX_TRIG for external power
    pinMode(AUX_TRIG, OUTPUT);
    digitalWrite(AUX_TRIG, HIGH);

    // Initialize I2C bus
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Scan I2C devices BEFORE initializing Ublox
    // scanI2CDevices();

    // Try initializing UBlox GNSS
    if (!init_ublox()) {
        Serial.println("UBlox GNSS setup failed. Check connections.");
    }

    // Scan I2C devices AFTER initializing Ublox
    scanI2CDevices();
}

void loop() {
//do nothing
}
