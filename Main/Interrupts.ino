#define BUTTON_PIN GPIO_NUM_39
#define HOLD_TIME 5000  // 5000 ms (5 seconds)

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool interruptTriggered = false;
long minTimeBetweenInterrupts = 250; // milliseconds
long lastInterrupt = -minTimeBetweenInterrupts;

long minTimeBetweenTilts = 2000;
long lastTilted = NOT_SPECIFIED;
bool tiltMessageShown = false;

volatile unsigned long pressStartTime = 0;
volatile bool buttonPressed = false;
volatile bool actionTriggered = false;

void IRAM_ATTR interruptHandler() {
  portENTER_CRITICAL_ISR(&mux);
  interruptTriggered = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR buttonISR() {
  if (digitalRead(BUTTON_PIN) == LOW) {  // Button pressed
    buttonPressed = true;  // Set flag
  } else {  // Button released
    buttonPressed = false;
    actionTriggered = false;  // Reset action so it can trigger again
  }
}


void setup_interrupts() {
  Serial.println("configuring pin GPIO_NUM_32 as interrupt...");
  pinMode(GPIO_NUM_32, INPUT_PULLDOWN);
  attachInterrupt(GPIO_NUM_32, interruptHandler, HIGH);

  Serial.println("configuring pin GPIO_NUM_39 as interrupt...");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);
}

void loop_interrupts() {
  if (interruptTriggered && (millis() - lastInterrupt > minTimeBetweenInterrupts)) {
    Serial.println("tilt sensor interrupt triggered! handling it...");
    lastInterrupt = millis();
    portENTER_CRITICAL(&mux);
    interruptTriggered = false;
    portEXIT_CRITICAL(&mux);

    lastTilted = millis();

    // If the tilt was recent, then show the device is tilted.
    // If it was a few seconds ago, then redraw everything.
  }

  if (lastTilted != NOT_SPECIFIED) { // only handle tilts if there has been any
    if ((millis() - lastTilted) < minTimeBetweenTilts) { // was the tilt recent?
      displayFit("I'm tilted!", 0, 0, displayWidth(), displayHeight(), 6);
      tiltMessageShown = true;
    } else if (tiltMessageShown) { // else not recent but message still shown?
      displayFit("Back up!", 0, 0, displayWidth(), displayHeight(), 6);
      tiltMessageShown = false;
      nextRefreshBalanceAndPayments();
    }
  }

  if (buttonPressed) {  // Check if the button is being held
    if (pressStartTime == 0) {
      pressStartTime = millis();  // Record when the button was first pressed
    }

    if (!actionTriggered && millis() - pressStartTime >= HOLD_TIME) {
      Serial.println("Button held for 5 seconds! Action triggered.");
      actionTriggered = true;  // Prevent repeated triggering
      displayFit("User button was pushed for more than 3s, starting Access Point configuration mode!", 0, 0, displayWidth(), displayHeight(), MAX_FONT);
      piggyMode = PIGGYMODE_STARTING_AP;
    }
  } else {
    pressStartTime = 0;  // Reset timer when the button is released
  }
}
