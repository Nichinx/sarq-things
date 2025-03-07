#include <HardwareSerial.h>

#define GSM_RX 16    // UART1 RX (ESP32 receiving from GSM)
#define GSM_TX 17    // UART1 TX (ESP32 sending to GSM)
#define GSM_RST 25   // GSM Reset Pin
#define GSM_INT 35   // GSM Interrupt (Ring Indicator)
#define AUX_TRIG 26

HardwareSerial GSMSerial(1); // Use UART1 for GSM communication

void setup() {
  Serial.begin(115200);

 //trigger aux 3.3v
  pinMode(AUX_TRIG, OUTPUT);
  delay(500);
  digitalWrite(AUX_TRIG, HIGH);
  delay(500);

  GSMSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX); // RX, TX pins for UART1
  pinMode(GSM_RST, OUTPUT);
  pinMode(GSM_INT, INPUT);
  digitalWrite(GSM_RST, HIGH);
  delay(1000);

  Serial.println("Initializing GSM module...");
  if (GSMInit()) {
    Serial.println("GSM module initialized successfully.");
  } else {
    Serial.println("GSM module initialization failed.");
  }
}

void loop() {
  Serial.println("Select an option:");
  Serial.println("1. Print CSQ");
  Serial.println("2. Test send SMS");
  while (Serial.available() == 0) { delay(100); }

  String option = Serial.readStringUntil('\n');
  option.trim(); // Remove any leading/trailing whitespace

  // switch (option) {
  //   case '1':
  //     printCSQ();
  //     break;
  //   case '2':
  //     testSendSMS();
  //     break;
  //   default:
  //     Serial.println("Invalid option.");
  // }
  if (option == "1") {
    printCSQ();
  } else if (option == "2") {
    testSendSMS();
  } else {
    Serial.println("Invalid option.");
  }

  delay(1000);
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

  // Wait for the GSM module to respond
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

void printCSQ() {
  char csqBuffer[50];
  GSMSerial.write("AT+CSQ\r");
  delay(1000);
  GSMAnswer(csqBuffer, sizeof(csqBuffer));
  Serial.print("CSQ: ");
  Serial.println(csqBuffer);
}

// void testSendSMS() {
//   const char* phoneNumber = "639954127577"; // Replace with actual phone number
//   const char* message = "Hello from ESP32!";
//   char command[50];
//   sprintf(command, "AT+CMGS=\"%s\"\r", phoneNumber);
//   GSMSerial.write(command);
//   delay(1000);
//   GSMSerial.write(message);
//   GSMSerial.write(26); // ASCII code for CTRL+Z
//   Serial.println("SMS sent.");
// }

void testSendSMS() {
  const char* phoneNumber = "639954127577"; // Replace with actual phone number
  char message[160];
  Serial.println("Enter SMS message:");
  readSerialInput(message, sizeof(message));
  Serial.print("Sending SMS: ");
  Serial.println(message);
  char command[50];
  sprintf(command, "AT+CMGS=\"%s\"\r", phoneNumber);
  GSMSerial.write(command);
  delay(1000);
  GSMSerial.write(message);
  GSMSerial.write(26); // ASCII code for CTRL+Z
  Serial.println("SMS sent.");
}

void readSerialInput(char* buffer, int bufferSize) {
  int index = 0;
  while (index < bufferSize - 1) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
      buffer[index++] = c;
    }
  }
  buffer[index] = '\0';
}

// void GSMAnswer(char* responseBuffer, int bufferLength) {
//   int bufferIndex = 0;
//   unsigned long waitStart = millis();

//   for (int i = 0; i < bufferLength; i++) {
//     responseBuffer[i] = 0x00;
//   }
//   for (int j = 0; j < bufferLength && GSMSerial.available() > 0; j++) {
//     responseBuffer[j] = GSMSerial.read();
//   }
//   if (strlen(responseBuffer) > 0) Serial.println(responseBuffer);
//   if (strlen(responseBuffer) >= bufferLength) {
//     responseBuffer[bufferLength - 1] = 0x00;
//   } else {
//     responseBuffer[strlen(responseBuffer)] = 0x00;
//   }
// }

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