#define RAIN_INT 36  // Rain Gauge Interrupt Pin (ESP32)
volatile int rainCount = 0; // Stores number of interrupts

void IRAM_ATTR rainISR() {
    rainCount++; // Increment on each interrupt
}

void setup() {
    Serial.begin(115200);
    pinMode(RAIN_INT, INPUT_PULLUP);
    
    // Attach Interrupt to detect rain gauge pulses
    attachInterrupt(digitalPinToInterrupt(RAIN_INT), rainISR, FALLING);
    
    Serial.println("Rain Gauge Monitoring Initialized...");
}

void loop() {
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {  // Print every 5 seconds
        Serial.printf("Rain Gauge Pulses: %d\n", rainCount);
        lastPrint = millis();
    }
}
