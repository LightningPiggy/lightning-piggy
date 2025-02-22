#define BUTTON_PIN GPIO_NUM_39
#define HOLD_TIME 3000  // 3 seconds

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

  if (lastTilted != NOT_SPECIFIED && piggyMode == PIGGYMODE_STARTED_STA) { // only handle tilts if there has been any, and if we're in regular idle mode, waiting for payments to come in
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
      Serial.println("Recording pressStartTime");
      pressStartTime = millis();  // Record when the button was first pressed
    }

    long pressDuration = millis() - pressStartTime;
    if (!actionTriggered) {
      actionTriggered = true;  // Prevent repeated triggering
      if (pressDuration > minTimeBetweenInterrupts && pressDuration < HOLD_TIME) {
        Serial.println("Handling short button press");
        moveOnAfterSleepBootSlogan();
      } else if (pressDuration >= HOLD_TIME) {
        // handle long press
        Serial.println("Handling long button press");
        if (piggyMode != PIGGYMODE_STARTING_AP && piggyMode != PIGGYMODE_STARTED_AP) {
          displayFit("User button was pushed for more than 3s, starting Access Point configuration mode!", 0, 0, displayWidth(), displayHeight(), MAX_FONT);
          piggyMode = PIGGYMODE_STARTING_AP;
        } else {
          displayFit("User button was pushed while already in configuration mode, going back to station mode!", 0, 0, displayWidth(), displayHeight(), MAX_FONT);
          piggyMode = PIGGYMODE_STARTING_STA;
        }
      }
    }
  } else { // button not pressed
    pressStartTime = 0;  // Reset timer when the button is released
  }
}
