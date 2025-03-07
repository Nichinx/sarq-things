#define RAIN_INT 36  // Rain Gauge Interrupt Pin (ESP32)
volatile int rainCount = 0; // Stores number of interrupts
uint8_t rainCollectorType = 0; // 0: Pronamic (0.5mm/tip), 1: Davis (0.2mm/tip)
volatile bool printFlag = false; // Flag to indicate printing is needed

void IRAM_ATTR rainISR() {
    rainCount++; // Increment on each interrupt
    printFlag = true; // Set flag to indicate printing is needed
}

void setup() {
    Serial.begin(115200);
    pinMode(RAIN_INT, INPUT_PULLUP);
    
    // Attach Interrupt to detect rain gauge pulses
    attachInterrupt(digitalPinToInterrupt(RAIN_INT), rainISR, FALLING);
    
    Serial.println("Rain Gauge Monitoring Initialized...");
    selectRainCollectorType();
}

// void loop() {
//     static unsigned long lastPrint = 0;
//     if (millis() - lastPrint > 5000) {  // Print every 5 seconds
//         Serial.printf("Rain Gauge Pulses: %d\n", rainCount);
//         lastPrint = millis();
//     }

//     if (Serial.available() > 0) {
//         int command = Serial.parseInt();
//         if (command == 1) {
//             printRainInMM();
//         } else if (command == 2) {
//             resetRainTips();
//         }
//     }
// }

void loop() {
    if (printFlag) {
        Serial.printf("Rain tips: %d\n", rainCount);
        printFlag = false; // Reset flag after printing
    }

    if (Serial.available() > 0) {
        int command = Serial.parseInt();
        if (command == 1) {
            printRainInMM();
        } else if (command == 2) {
            resetRainTips();
        } else if (command == 3) {
            selectRainCollectorType();
        }
    }
}

void selectRainCollectorType() {
    Serial.println("Select Rain Collector Type:");
    Serial.println("0: Pronamic (0.5mm/tip)");
    Serial.println("1: Davis (0.2mm/tip)");
    unsigned long startTime = millis();
    while (millis() - startTime < 60000) { // Wait for up to 60 seconds for input
        if (Serial.available() > 0) {
            int type = Serial.parseInt();
            if (type == 0 || type == 1) {
                rainCollectorType = type;
                Serial.printf("Rain Collector Type set to: %d\n", rainCollectorType);
                return;
            } else {
                Serial.println("Invalid value, please enter 0 or 1.");
            }
        }
    }
    Serial.println("Timeout: Rain Collector Type unchanged.");
}

void printRainInMM() {
    float mmPerTip = (rainCollectorType == 0) ? 0.5 : 0.2;
    float rainInMM = rainCount * mmPerTip;
    Serial.printf("Rain in mm: %.2f\n", rainInMM);
}

void resetRainTips() {
    rainCount = 0;
    Serial.println("Rain tips reset.");
}