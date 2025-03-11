#include <SPI.h>
#include <RadioLib.h>

#define LORA_CS 5
#define LORA_RST 4
#define LORA_DIO0 27
#define AUX_TRIG 26  

SX1278 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, -1);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 LoRa RF95 Test");

    pinMode(AUX_TRIG, OUTPUT);
    delay(100);
    digitalWrite(AUX_TRIG, HIGH);

    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);

    int status = radio.begin(433.0, 125.0, 9, 7, 0x34, 8);
    if (status != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa init failed! Error: ");
        Serial.println(status);
        while (true);
    }
    Serial.println("LoRa initialized!");

    radio.setOutputPower(23);
    radio.setSpreadingFactor(7);
    radio.setCodingRate(5);
    radio.setBandwidth(125.0);
    radio.setPreambleLength(8);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        Serial.print("Sending: ");
        Serial.println(input);

        int txStatus = radio.transmit(input);
        if (txStatus == RADIOLIB_ERR_NONE) {
            Serial.println("Message Sent!");
        } else {
            Serial.print("Send Failed! Error: ");
            Serial.println(txStatus);
        }

    } else {
        String message;
        int state = radio.receive(message);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.print("Received: ");
            Serial.println(message);
        }
    }
}
