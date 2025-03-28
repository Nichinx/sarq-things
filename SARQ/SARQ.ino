#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <HardwareSerial.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <esp_adc_cal.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

// Define pins and constants
#define LORA_CS 5
#define LORA_RST 4
#define LORA_DIO0 27

#define AUX_TRIG 26
#define VMON_PIN 39

#define RXD2 33
#define TXD2 32

#define SDA_PIN 21
#define SCL_PIN 22

#define GSM_RX 16
#define GSM_TX 17
#define GSM_RST 25
#define GSM_INT 35

#define RAIN_INT 36

#define BAUDRATE 115200

#define ADC_MAX_VALUE 4095
#define REF_VOLTAGE 3.3
#define DEFAULT_VREF 1100

// Global variables
HardwareSerial mySerial(2);
HardwareSerial GSMSerial(1);
RTC_DS3231 rtc;
SFE_UBLOX_GNSS myGNSS;
esp_adc_cal_characteristics_t *adc_chars;
volatile int rainCount = 0;
uint8_t rainCollectorType = 0;
bool gsmInitialized = false;
volatile bool rainInterruptFlag = false;

void setup() {
    Serial.begin(BAUDRATE);
    mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
    GSMSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(AUX_TRIG, OUTPUT);
    digitalWrite(AUX_TRIG, HIGH);
    pinMode(RAIN_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(RAIN_INT), rainISR, FALLING);

    adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
   
    rtc.begin();
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // init_ublox();
    GSMInit();

    // Initialize LoRa
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa init failed! Continuing without LoRa functionality.");
    } else {
        LoRa.setTxPower(23);
        LoRa.setSpreadingFactor(7);
        LoRa.setSignalBandwidth(125E3);
        LoRa.setCodingRate4(5);
        LoRa.setPreambleLength(8);
    }

    Serial.println("Comprehensive Test Initialized. Select an option:");
    printOptions();
}

// void loop() {
//     if (Serial.available()) {
//         char option = Serial.read();
//         option = toupper(option); // Convert to uppercase
//         while (Serial.available() && Serial.read() != '\n');
//         switch (option) {
//             case 'A': printOptions(); break;
//             case 'B': serialEchoTest(); break;
//             case 'C': scanI2CDevices(); break;
//             case 'D': printDateTime(); break;
//             case 'E': changeDateTime(); break;
//             case 'F': printRainTips(); break;
//             case 'G': resetRainTips(); break;
//             case 'H': changeRainGaugeType(); break;
//             case 'I': printCSQ(); break;
//             case 'J': sendSMS(); break;
//             case 'K': loraSendTest(); break;
//             case 'L': loraReceiveTest(); break;
//             case 'M': printVoltage(); break;
//             default: Serial.println("Invalid option. Please select a valid option."); break;
//         }
//     }

//     // Handle the rain interrupt flag
//     if (rainInterruptFlag) {
//         rainInterruptFlag = false; // Clear the flag
//         Serial.printf("No. of tips: %d\n", rainCount); // Print number of tips
//     }
// }

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 1) {
            char option = toupper(input.charAt(0)); // Convert to uppercase
            switch (option) {
                case 'A': printOptions(); break;
                case 'B': serialEchoTest(); break;
                case 'C': scanI2CDevices(); break;
                case 'D': printDateTime(); break;
                case 'E': changeDateTime(); break;
                case 'F': printRainTips(); break;
                case 'G': resetRainTips(); break;
                case 'H': changeRainGaugeType(); break;
                case 'I': printCSQ(); break;
                case 'J': sendSMS(); break;
                case 'K': loraSendTest(); break;
                case 'L': loraReceiveTest(); break;
                case 'M': printVoltage(); break;
                default: Serial.println("Invalid option. Please select a valid option."); break;
            }
        } else {
            Serial.println("Invalid input. Please enter a single letter.");
        }
    }

    // Handle the rain interrupt flag
    if (rainInterruptFlag) {
        rainInterruptFlag = false; // Clear the flag
        Serial.printf("No. of tips: %d\n", rainCount); // Print number of tips
    }
}

void printOptions() {
    Serial.println("");
    Serial.println("Select an option:");
    Serial.println("A. Print options");
    Serial.println("B. Serial echo test (SS pins)");
    Serial.println("C. Scan I2C devices");
    Serial.println("D. Print datetime");
    Serial.println("E. Change datetime");
    Serial.println("F. Print tips");
    Serial.println("G. Reset tips");
    Serial.println("H. Change rain gauge type");
    Serial.println("I. Print CSQ");
    Serial.println("J. Send SMS");
    Serial.println("K. LoRa Send Test");
    Serial.println("L. LoRa Receive Test");
    Serial.println("M. Print voltage (analog read, vin calculated, calibrated vin)");
    Serial.println("");
}

void serialEchoTest() {
    Serial.println("ESP32 UART Echo Test (Type something and press Enter, type 'exit' to quit)");
    while (true) {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.equalsIgnoreCase("exit")) {
                Serial.println("Exiting Serial Echo Test.");
                break;
            }
            Serial.print("tx: ");
            Serial.println(input);
            mySerial.println(input);
            Serial.println(""); Serial.println(">"); Serial.println("");
            delay(100);
        }
        if (mySerial.available()) {
            String received = mySerial.readStringUntil('\n');
            received.trim();
            Serial.print("rx: ");
            Serial.println(received);
        }
    }
}

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
        Serial.println("External I2C is working.");
    } else {
        Serial.println("No I2C devices found!");
    }
}

void printDateTime() {
    DateTime now = rtc.now();
    Serial.print("Date & Time: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
}

void changeDateTime() {
    Serial.println("Enter date & time in the format: YYYY,MM,DD,HH,MM,SS or type 'exit' to quit");
    unsigned long startTime = millis();
    String input = "";

    // Wait for user input with a 60-second timeout
    while (millis() - startTime < 60000) { // 60-second timeout
        if (Serial.available()) {
            input = Serial.readStringUntil('\n');
            input.trim();
            break;
        }
    }

    if (input.isEmpty()) {
        Serial.println("No input received. Exiting Change DateTime.");
        return;
    }

    if (input.equalsIgnoreCase("exit")) {
        Serial.println("Exiting Change DateTime.");
        return;
    }

    int year, month, day, hour, minute, second;
    if (sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        rtc.adjust(DateTime(year, month, day, hour, minute, second));
        Serial.println("RTC updated successfully.");
    } else {
        Serial.println("Invalid input. RTC not updated.");
    }

    // Clear any remaining input in the serial buffer
    while (Serial.available()) Serial.read();
}

void printRainTips() {
    float mmPerTip = (rainCollectorType == 0) ? 0.5 : 0.2;
    float rainInMM = rainCount * mmPerTip;
    Serial.printf("Rain in mm: %.2f\n", rainInMM);
}

void changeRainGaugeType() {
    Serial.println("Select Rain Collector Type:");
    Serial.println("0: Pronamic (0.5mm/tip)");
    Serial.println("1: Davis (0.2mm/tip)");
    while (true) {
        if (Serial.available() > 0) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.equalsIgnoreCase("exit")) {
                Serial.println("Exiting Change Rain Gauge Type.");
                return;
            }
            int type = input.toInt();
            if (type == 0 || type == 1) {
                rainCollectorType = type;
                Serial.printf("Rain Collector Type set to: %d\n", rainCollectorType);
                return;
            } else {
                Serial.println("Invalid value, please enter 0 or 1.");
            }
        }
    }
}

void resetRainTips() {
    rainCount = 0;
    Serial.println("Rain tips reset.");
}

void rainISR() {
    rainCount++;
    rainInterruptFlag = true; // Set the flag to indicate an interrupt has occurred
    // Serial.printf("No. of tips: %d\n", rainCount); // Print number of tips when interrupt is detected  
}

void loraSendTest() {
    Serial.println("LoRa Send Test (Type 'exit' to quit)");
    while (true) {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.equalsIgnoreCase("exit")) {
                Serial.println("Exiting LoRa Send Test.");
                break;
            }
            Serial.print("Message to send: ");
            Serial.println(input);
            LoRa.beginPacket();
            LoRa.write(input.length());
            LoRa.print(input);
            LoRa.endPacket();
            Serial.println("Message Sent!");
            delay(2000);
        }
    }
}

void loraReceiveTest() {
    Serial.println("LoRa Receive Test (Type 'exit' to quit)");
    while (true) {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.equalsIgnoreCase("exit")) {
                Serial.println("Exiting LoRa Receive Test.");
                break;
            }
        }
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            int msgLength = LoRa.read();
            String received = "";
            while (LoRa.available() && received.length() < msgLength) {
                received += (char)LoRa.read();
            }
            Serial.print("Received: ");
            Serial.println(received);
        }
    }
}

float calculateVIN(uint32_t adcRaw) {
    uint32_t adcVoltage_mV = esp_adc_cal_raw_to_voltage(adcRaw, adc_chars);
    float adcVoltage = adcVoltage_mV / 1000.0;
    float gain = 1.0 + (200000.0 / 10000.0);
    float VIN = (adcVoltage / gain) * ((100000.0 + 10000000.0) / 100000.0);
    return VIN;
}

float calibrateVIN(float vin) {
    return (0.00121 * vin * vin * vin) - (0.0298 * vin * vin) + (1.228 * vin) - 0.627;
}

void printVoltage() {
    int rawADC = analogRead(VMON_PIN);
    uint32_t adcCalVoltage = esp_adc_cal_raw_to_voltage(rawADC, adc_chars);
    float VIN = calculateVIN(rawADC);
    float calibratedVIN = calibrateVIN(VIN);
    Serial.print("Raw ADC: ");
    Serial.print(rawADC);
    Serial.print(" | ESP ADC Cal Raw to Voltage: ");
    Serial.print(adcCalVoltage / 1000.0, 3);
    Serial.print(" | Calculated VIN: ");
    Serial.print(VIN, 3);
    Serial.print(" | Calibrated VIN: ");
    Serial.println(calibratedVIN, 3);
}

void printCSQ() {
    char csqBuffer[50];
    GSMSerial.write("AT+CSQ\r");
    delay(1000);
    GSMAnswer(csqBuffer, sizeof(csqBuffer));
    Serial.print("CSQ: ");
    Serial.println(csqBuffer);
}

void sendSMS() {
    char phoneNumber[20];
    char message[160];
    Serial.println("Enter SIM No. (in format 639XXXXXXXXX):");
    while (Serial.available()) Serial.read();
    while (!Serial.available());
    String phoneInput = Serial.readStringUntil('\n');
    phoneInput.trim();
    phoneInput.toCharArray(phoneNumber, 20);
    
    Serial.println("Enter SMS message:");
    while (Serial.available()) Serial.read();
    while (!Serial.available());
    String input = Serial.readStringUntil('\n');
    input.toCharArray(message, 160);
    
    Serial.print("Sending SMS to ");
    Serial.print(phoneNumber);
    Serial.print(": ");
    Serial.println(message);
    
    char command[50];
    sprintf(command, "AT+CMGS=\"%s\"\r", phoneNumber);
    GSMSerial.write(command);
    delay(1000);
    GSMSerial.write(message);
    GSMSerial.write(26);
    
    if (GSMWaitResponse("OK", 10000, true)) {
        Serial.println("SMS sent.");
    } else {
        Serial.println("Failed to send SMS.");
    }
}

bool GSMInit() {
    bool initOK = false;
    bool gsmSerial = false, GSMconfig = false, signalCOPS = false;
    unsigned long initStart = millis();
    const int initTimeout = 25000;
    uint8_t errorCount = 0;
    Serial.println("Connecting GSM to network...");
    GSMSerial.begin(9600);
    delay(300);
    digitalWrite(GSM_RST, HIGH);
    unsigned long gsmPowerOn = millis();
    char gsmInitResponse[100] = {0};
    do {
        memset(gsmInitResponse, 0, sizeof(gsmInitResponse));
        GSMAnswer(gsmInitResponse, sizeof(gsmInitResponse));
        if (strlen(gsmInitResponse) > 0 && strcmp(gsmInitResponse, "\n") != 0) {
            Serial.println(gsmInitResponse);
        }
        delay(300);
    } while (millis() - gsmPowerOn < 5000);
    while (!gsmSerial || !GSMconfig || !signalCOPS) {
        if (!gsmSerial) {
            GSMSerial.write("AT\r");
            if (GSMWaitResponse("OK", 1000, true)) {
                Serial.println("Serial comm ready!");
                GSMSerial.write("ATE0\r");
                gsmSerial = true;
            } else {
                errorCount++;
            }
        }
        if (gsmSerial && !GSMconfig) {
            GSMSerial.flush();
            delay(500);
            GSMSerial.write("AT+COPS=0,1\r");
            delay(500);
            GSMSerial.write("AT+CMGF=1\r");
            delay(500);
            GSMSerial.write("AT+IPR=0\r");
            delay(500);
            GSMSerial.write("AT+CNMI=0,0,0,0,0\r");
            if (GSMWaitResponse("OK", 5000, true)) {
                Serial.println("GSM module config OK!");
                GSMconfig = true;
            }
        }
        if (GSMconfig && !signalCOPS) {
            GSMSerial.flush();
            Serial.println("Checking GSM network signal...");
            GSMSerial.write("AT+COPS?\r");
            delay(1000);
            memset(gsmInitResponse, 0, sizeof(gsmInitResponse));
            if (GSMGetResponse(gsmInitResponse, sizeof(gsmInitResponse), "+COPS: 0,1,\"", 3000)) {
                char CSQval[10] = {0};
                if (readCSQ(CSQval)) {
                    signalCOPS = true;
                }
            }
            GSMSerial.write("AT+CREG?\r");
            delay(500);
            GSMSerial.write("AT+CPIN?\r");
            delay(500);
            GSMSerial.write("AT+CSQ\r");
            delay(500);
        }
        if (gsmSerial && GSMconfig && signalCOPS) {
            GSMSerial.write("AT&W\r");
            delay(500);
            initOK = true;
            break;
        }
        if (errorCount >= 5) {
            Serial.println("CHECK_GSM_SERIAL_OR_POWER");
            break;
        }
        if (millis() - initStart > initTimeout) {
            Serial.print("GSM_INIT: ");
            if (!GSMconfig) {
                Serial.println("GSM_MODULE_CONFIG_ERROR");
            } else if (!signalCOPS) {
                Serial.println("NETWORK_OR_SIM_ERROR");
            }
            break;
        }
    }
    GSMSerial.flush();
    return initOK;
}

void GSMAnswer(char* responseBuffer, int bufferLength) {
    int bufferIndex = 0;
    unsigned long waitStart = millis();
    for (int i = 0; i < bufferLength; i++) {
        responseBuffer[i] = 0x00;
    }
    while (GSMSerial.available() > 0 && bufferIndex < bufferLength - 1) {
        responseBuffer[bufferIndex++] = GSMSerial.read();
    }
    responseBuffer[bufferIndex] = '\0';
    if (strlen(responseBuffer) > 0) Serial.println(responseBuffer);
}

bool GSMWaitResponse(const char* targetResponse, int waitDuration, bool showResponse) {
    bool responseValid = false;
    char toCheck[50];
    char charBuffer;
    char responseBuffer[300];
    unsigned long waitStart = millis();
    strcpy(toCheck, targetResponse);
    toCheck[strlen(toCheck)] = 0x00;
    do {
        for (int i = 0; i < sizeof(responseBuffer); i++) {
            responseBuffer[i] = 0x00;
        }
        for (int j = 0; j < sizeof(responseBuffer) && GSMSerial.available() > 0; j++) {
            charBuffer = GSMSerial.read();
            if (charBuffer == '\n' || charBuffer == '\r') {
                responseBuffer[j] = 0x00;
                break;
            } else {
                responseBuffer[j] = charBuffer;
            }
        }
        if (strlen(responseBuffer) > 0 && responseBuffer != "\n") {
            if (showResponse) Serial.println(responseBuffer);
            if (strstr(responseBuffer, toCheck)) {
                responseValid = true;
                break;
            }
        }
        delay(50);
    } while (!responseValid && millis() - waitStart < waitDuration);
    return responseValid;
}

bool GSMGetResponse(char* responseContainer, int containerLen, const char* responseReq, int getDuration) {
    char toCheck[50];
    unsigned long waitStart = millis();
    char charBuffer;
    strcpy(toCheck, responseReq);
    toCheck[strlen(toCheck)] = 0x00;
    do {
        delay(300);
        for (int i = 0; i < containerLen; i++) {
            responseContainer[i] = 0x00;
        }
        for (int j = 0; j < containerLen && GSMSerial.available() > 0; j++) {
            charBuffer = GSMSerial.read();
            if (responseContainer[j] == '\n' || responseContainer[j] == '\r') {
                break;
            } else {
                responseContainer[j] = charBuffer;
            }
        }
        if (strlen(responseContainer) == containerLen) {
            responseContainer[containerLen - 1] = 0x00;
        } else {
            responseContainer[strlen(responseContainer)] = 0x00;
        }
        if (strlen(responseContainer) > 0) {
            if (strstr(responseContainer, toCheck)) {
                return true;
            }
        }
    } while (millis() - waitStart < getDuration);
    return false;
}

bool readCSQ(char* csqContainer) {
    bool responseValid = false;
    char csqSerialBuffer[50];

    GSMSerial.write("AT\r");
    delay(200);
    GSMSerial.flush();
    GSMSerial.write("AT+CSQ\r");
    delay(1000);
    GSMAnswer(csqSerialBuffer, sizeof(csqSerialBuffer));
    sprintf(csqContainer, csqSerialBuffer);
    if (strstr(csqSerialBuffer, "+CSQ:")) responseValid = true;
    return responseValid;
}

bool init_ublox() {
    Serial.println("Initializing UBlox GNSS...");
    for (int attempts = 0; attempts < 3; attempts++) {
        if (myGNSS.begin(Wire)) {
            Serial.println("UBlox GNSS initialized.");
            myGNSS.setI2COutput(COM_TYPE_UBX);
            myGNSS.setNavigationFrequency(5);
            myGNSS.setHighPrecisionMode(true);
            myGNSS.powerSaveMode(false);
            Serial.println("UBlox GNSS configured successfully.");
            return true;
        }
        Serial.println(F("UBlox GNSS not detected at default I2C address. Retrying..."));
        delay(1000);
    }
    Serial.println("Failed to initialize UBlox GNSS.");
    return false;
}