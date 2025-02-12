
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool interruptTriggered = false;
long minTimeBetweenInterrupts = 250; // milliseconds
long lastInterrupt = -minTimeBetweenInterrupts;

long minTimeBetweenTilts = 2000;
long lastTilted = NOT_SPECIFIED;
bool tiltMessageShown = false;

volatile unsigned long userButtonPushMicros  = 0;
volatile unsigned long userButtonReleaseMicros  = 0;

void IRAM_ATTR interruptHandler() {
  portENTER_CRITICAL_ISR(&mux);
  interruptTriggered = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR interruptHandlerUserButton() {
  if (digitalRead(GPIO_NUM_39) == HIGH) {
        userButtonReleaseMicros = micros();
    } else {
        userButtonPushMicros = micros();
    }
}


void setup_interrupts() {
  Serial.println("configuring pin GPIO_NUM_32 as interrupt...");
  pinMode(GPIO_NUM_32, INPUT_PULLDOWN);
  attachInterrupt(GPIO_NUM_32, interruptHandler, HIGH);

  Serial.println("configuring pin GPIO_NUM_39 as interrupt...");
  pinMode(GPIO_NUM_39, INPUT_PULLUP);
  attachInterrupt(GPIO_NUM_39, interruptHandlerUserButton, CHANGE);
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

  unsigned long pushDuration = userButtonReleaseMicros - userButtonPushMicros;
  if (pushDuration > 3*1000*1000 && pushDuration < 30*1000*1000) {
    displayFit("User button was pushed for more than 3s, starting Access Point configuration mode!", 0, 0, displayWidth(), displayHeight(), MAX_FONT);
    piggyMode = PIGGYMODE_STARTING_AP;
    userButtonPushMicros = 0;
    userButtonReleaseMicros = 0;
  }

}
