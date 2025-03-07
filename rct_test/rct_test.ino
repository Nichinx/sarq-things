#include <Wire.h>
#include "RTClib.h"
#include <TimeLib.h>

RTC_DS3231 rtc;

void i2cScan() {
    byte error, address;
    int nDevices = 0;
    
    Serial.println("Scanning for I2C devices...");
    
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            Serial.println(address, HEX);
            nDevices++;
        }
    }
    
    if (nDevices == 0) {
        Serial.println("No I2C devices found. RTC might not be connected.");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    delay(100);

    i2cScan(); // Check if RTC is even detected

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC lost power, setting default time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    delay(500);
    Serial.println("Select an option:");
    Serial.println("1 - Set RTC time manually");
    // Serial.println("2 - Sync RTC with system time");
    Serial.println("3 - Print RTC time");
    
    while (!Serial.available()); // Wait for input

    char option = Serial.read();
    while (Serial.available() && Serial.read() != '\n'); // Consume newline

    if (option == '1') {
        setRTCManually();
    // } else if (option == '2') {
    //     syncRTCTime();
    } else if (option == '3') {
        // Serial.println("Printing RTC time...");
        printRTCTime();
        // break;
    // } else {
    //     Serial.println("Invalid option. Please select 1, or 3.");
    }
}

void loop() {
    if (Serial.available()) {
        char option = Serial.read();
        while (Serial.available() && Serial.read() != '\n'); // Consume newline

        if (option == '1') {
            setRTCManually();
        // } else if (option == '2') {
        //     syncRTCTime();
        } else if (option == '3') {
            printRTCTime();
        } else {
            Serial.println("Invalid option. Please select 1, 2, or 3.");
        }
    }
}

void setRTCManually() {
    Serial.println("Enter date & time in the format: YYYY,MM,DD,HH,MM,SS");

    while (Serial.available()) Serial.read(); // Clear buffer

    while (!Serial.available()); // Wait for input

    String input = Serial.readStringUntil('\n');
    int year, month, day, hour, minute, second;

    if (sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        rtc.adjust(DateTime(year, month, day, hour, minute, second));
        Serial.println("RTC updated successfully.");
    } else {
        Serial.println("Invalid input. RTC not updated.");
    }

    while (Serial.available()) Serial.read(); // Clear buffer after input
}

void syncRTCTime() {
    time_t t = now();
    rtc.adjust(DateTime(year(t), month(t), day(t), hour(t), minute(t), second(t)));
    Serial.println("RTC synced with system time.");
}

void printRTCTime() {
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
