const int PIR_PIN = 2;
const int LED_PIN = 9;
const int POT_PIN = A0;

// Combo: 3 digits (change these to your code)
const int SECRET_CODE[] = {0, 7, 0};
const int CODE_LENGTH = 3;
int enteredCode[3] = {-1, -1, -1};
int codeIndex = 0;

// Zone detection
int currentZone = -1;
int lastZone = -1;
unsigned long zoneHoldStart = 0;
const int HOLD_TIME = 3000;

// NEW: must move away before next digit
bool waitingForMove = false;
int lockedZone = -1;

// System state
bool armed = false;
unsigned long lastBlinkTime = 0;
bool ledState = false;

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("Warming up PIR sensor (30 sec)...");
  delay(30000);
  Serial.println("System ready. Waiting for motion...");
}

void loop() {
  int potValue = analogRead(POT_PIN);
  currentZone = map(potValue, 0, 1023, 0, 9);
  
  if (!armed && digitalRead(PIR_PIN) == HIGH) {
    armed = true;
    Serial.println("");
    Serial.println("=========================");
    Serial.println(">>> SOMEONE AT THE DOOR <<<");
    Serial.println("=========================");
    Serial.println("");
    Serial.println("Enter 3-digit code to disarm.");
    Serial.println("Hold 1 sec, then MOVE to next digit.");
    
    codeIndex = 0;
    waitingForMove = false;
    for (int i = 0; i < CODE_LENGTH; i++) {
      enteredCode[i] = -1;
    }
  }
  
  if (armed) {
    if (millis() - lastBlinkTime > 200) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      lastBlinkTime = millis();
    }
    
    handleComboEntry();
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(50);
}

void handleComboEntry() {
  // If waiting for user to move away, check if they did
  if (waitingForMove) {
    Serial.print("Zone: ");
    Serial.print(currentZone);
    Serial.println(" - Move away to enter next digit");

    if (abs(currentZone - lockedZone) >= 2) {
      // Moved at least 2 zones away - ready for next digit
      waitingForMove = false;
      lastZone = currentZone;
      zoneHoldStart = millis();
      Serial.println("Ready for next digit!");
    }
    return;  // Don't process until they move
  }

  // Normal zone detection
  if (currentZone != lastZone) {
    zoneHoldStart = millis();
    lastZone = currentZone;
  }

  unsigned long holdTime = millis() - zoneHoldStart;
  int secondsLeft = (HOLD_TIME - holdTime) / 1000 + 1;

  Serial.print("Zone: ");
  Serial.print(currentZone);
  Serial.print(" - Hold for ");
  Serial.print(secondsLeft);
  Serial.println("s to lock");

  if (holdTime > HOLD_TIME) {
    // Lock in this digit
    enteredCode[codeIndex] = currentZone;
    Serial.println("");
    Serial.print(">>> LOCKED Digit ");
    Serial.print(codeIndex + 1);
    Serial.print(": ");
    Serial.println(currentZone);
    Serial.println("");
    codeIndex++;

    // Now wait for user to move away
    waitingForMove = true;
    lockedZone = currentZone;

    if (codeIndex >= CODE_LENGTH) {
      checkCode();
    }
  }
}

void checkCode() {
  bool correct = true;
  for (int i = 0; i < CODE_LENGTH; i++) {
    if (enteredCode[i] != SECRET_CODE[i]) {
      correct = false;
      break;
    }
  }
  
  if (correct) {
    Serial.println("");
    Serial.println("*** DISARMED ***");
    Serial.println("");
    
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
    
    armed = false;
  } else {
    Serial.println("");
    Serial.println("WRONG CODE - Try again");
    Serial.println("");
    
    codeIndex = 0;
    waitingForMove = false;
    for (int i = 0; i < CODE_LENGTH; i++) {
      enteredCode[i] = -1;
    }
  }
}