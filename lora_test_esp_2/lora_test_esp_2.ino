#include <SPI.h>
#include <LoRa.h>

#define LORA_CS 5
#define LORA_RST 4
#define LORA_DIO0 27
#define AUX_TRIG 26  

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 LoRa Test");

    pinMode(AUX_TRIG, OUTPUT);
    digitalWrite(AUX_TRIG, HIGH);

    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa init failed!");
        while (true);
    }
    Serial.println("LoRa initialized!");

    LoRa.setTxPower(23);
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setPreambleLength(8);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();  // Remove unwanted characters

        Serial.print("Sending: ");
        Serial.println(input);

        LoRa.beginPacket();
        LoRa.write(input.length());  // Send packet length first
        LoRa.print(input);
        LoRa.endPacket();

        Serial.println("Message Sent!");
        delay(2000);
    } else {
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            int msgLength = LoRa.read();  // Read expected length
            String received = "";

            while (LoRa.available() && received.length() < msgLength) {
                received += (char)LoRa.read();
            }

            Serial.print("Received: ");
            Serial.println(received);
        }
    }
}
