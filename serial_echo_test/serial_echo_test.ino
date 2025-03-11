#define RXD2 33  // SS_RX (GPIO33)
#define TXD2 32  // SS_TX (GPIO32)

HardwareSerial mySerial(2);  // Use UART2

void setup() {
    Serial.begin(115200);   // USB Serial for debugging
    mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

    Serial.println("ESP32 UART Echo Test (Type something and press Enter)");
}

void loop() {
    // Check if user entered data in Serial Monitor
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');  // Read user input
        input.trim();  // Remove newline characters
        Serial.print("tx: ");
        Serial.println(input);  // Print sent message
        mySerial.println(input);  // Send input through UART2
        Serial.println(""); Serial.println(">"); Serial.println("");
        delay(100);
    }

    // Read data received from UART2 and print to Serial Monitor
    if (mySerial.available()) {
        String received = mySerial.readStringUntil('\n');
        received.trim();  // Remove newline characters
        Serial.print("rx: ");
        Serial.println(received);  // Print received message
    }
}
