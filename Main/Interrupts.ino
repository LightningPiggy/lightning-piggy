
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool interruptTriggered = false;
long minTimeBetweenInterrupts = 250; // milliseconds
long lastInterrupt = -minTimeBetweenInterrupts;

long minTimeBetweenTilts = 2000;
long lastTilted = NOT_SPECIFIED;
bool tiltMessageShown = false;

void IRAM_ATTR interruptHandler()
{
  portENTER_CRITICAL_ISR(&mux);
  interruptTriggered = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup_interrupts()
{
  Serial.println("configuring pin 32 as interrupt...");
  pinMode(GPIO_NUM_32, INPUT_PULLDOWN);
  attachInterrupt(GPIO_NUM_32, interruptHandler, HIGH);
}

void loop_interrupts()
{
  if (interruptTriggered && (millis() - lastInterrupt > minTimeBetweenInterrupts))
  {
    Serial.println("interrupt triggered! handling it...");
    lastInterrupt = millis();
    portENTER_CRITICAL(&mux);
    interruptTriggered = false;
    portEXIT_CRITICAL(&mux);

    lastTilted = millis();

    // If the tilt was recent, then show the device is tilted.
    // If it was a few seconds ago, then redraw everything.
  }

  if (lastTilted != NOT_SPECIFIED)
  { // only handle tilts if there has been any
    if ((millis() - lastTilted) < minTimeBetweenTilts)
    { // was the tilt recent?
      displayFit("I'm tilted!", 0, 0, displayWidth(), displayHeight(), 6);
      tiltMessageShown = true;
    }
    else if (tiltMessageShown)
    { // else not recent but message still shown?
      displayFit("Back up!", 0, 0, displayWidth(), displayHeight(), 6);
      tiltMessageShown = false;
      nextRefreshBalanceAndPayments();
    }
  }
}
