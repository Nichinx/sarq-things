#include <SPI.h>
#include <RH_RF95.h>

#define LORA_CS 8
#define LORA_RST 4
#define LORA_DIO0 3

RH_RF95 rf95(LORA_CS, LORA_DIO0);

void setup() {
    Serial.begin(115200);
    Serial.println("Feather M0 LoRa RF95 Test");

    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);

    if (!rf95.init()) {
        Serial.println("LoRa init failed!");
        while (true);
    }
    Serial.println("LoRa initialized!");

    rf95.setFrequency(433.0);
    rf95.setTxPower(23, false);
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
    rf95.setPreambleLength(8);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();  

        Serial.print("Sending: ");
        Serial.println(input);

        uint8_t msgLength = input.length();
        uint8_t data[msgLength + 1];
        data[0] = msgLength;  
        input.getBytes(data + 1, msgLength + 1);

        rf95.send(data, msgLength + 1);
        rf95.waitPacketSent();
        Serial.println("Message Sent!");
        delay(2000);
    } else {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);

        if (rf95.available()) {
            if (rf95.recv(buf, &len)) {
                int msgLength = buf[0];  
                buf[msgLength + 1] = '\0';

                Serial.print("Received: ");
                Serial.println((char*)(buf + 1));
            }
        }
    }
}
