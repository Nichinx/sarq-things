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
    }
  }

  if (deviceFound) {
    Serial.println("external i2c working");
  } else {
    Serial.println("No I2C device found!");
  }
}

void init_ublox() {
  Serial.println("Initializing UBlox GNSS...");

  for (int x = 0; x < 10; x++) {  // 10 retries
    if (myGNSS.begin(Wire) == false) {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
      delay(1000);
    } else {
      Serial.println("ublox initialized");
      break;
    }
  }

  // Configure GNSS module settings
  myGNSS.setI2COutput(COM_TYPE_UBX); // Output only UBX messages
  myGNSS.setNavigationFrequency(5);  // Set GNSS update rate to 5Hz
  myGNSS.setHighPrecisionMode(true);  
  myGNSS.powerSaveMode(true);

  Serial.println("UBlox GNSS configured successfully.");
}

void setup() {
  Serial.begin(BAUDRATE);
  
  //trigger aux 3.3v
  pinMode(AUX_TRIG, OUTPUT);
  delay(500);
  digitalWrite(AUX_TRIG, HIGH);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);

  // Try initializing UBlox if it's found
  if (myGNSS.begin(Wire)) {
    Serial.println("ublox initialized");
  }

  scanI2CDevices();  // Scan and print detected I2C devices
}

void loop() {
  // Add any GNSS data reading if needed
}
