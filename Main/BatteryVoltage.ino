// Keep a rolling average of the last 10 voltages
double averageVoltage = 0;
int averageVoltageN = 0;

double lastVoltage = NOT_SPECIFIED;

double approxRollingAverage(double avg, double input)
{
  averageVoltageN++;
  if (averageVoltageN > 10)
    averageVoltageN = 10;
  avg -= avg / averageVoltageN;
  avg += input / averageVoltageN;
  // Serial.print("approxRollingAverage = " + String(averageVoltage,4));
  return avg;
}

double getAverageBatteryVoltage()
{
  return averageVoltage;
}

double getLastVoltage()
{
  return lastVoltage;
}

// Returns the VBAT value.
// If we're not on battery but USB powered, then it returns a negative VBAT value.
double getBatteryVoltage()
{
  const int nrReads = 4;

  int totalDiff = 0;

  int prevValue = analogRead(35);
  delay(10);
  int totalLevel = prevValue;

  for (int multiread = 0; multiread < nrReads; multiread++)
  {
    int value = analogRead(35);
    totalLevel += value;
    totalDiff += abs(value - prevValue);
    delay(100);
  }

  totalLevel = totalLevel / (nrReads + 1); // one additional read is done for the prevValue
  totalDiff = totalDiff / nrReads;
  // Serial.println("Average battery level: " + String(totalLevel));
  // Serial.println("Average battery level diff: " + String(totalDiff));

  double voltage = (totalLevel * 1.72) / 1000;
  if (totalDiff > 32)
    voltage *= -1; // The battery has a smoothing effect so lots of variance between levels means we're not on battery.

  lastVoltage = voltage;
  averageVoltage = approxRollingAverage(averageVoltage, voltage);
  // return 3.7; // for testing low battery situation
  return voltage;
}

/*
 * 4.2 - 3.7 = 0.5 * 200 = 100 (%)
 *
 * Precentage = (V - 3.7) * 200
 * Voltage = (percent / 200) + 3.7
 */
int batteryVoltageToPercent(double voltage)
{
  voltage = voltage - 3.7;
  voltage = voltage * 200;
  if (voltage < 0)
  {
    return 0;
  }
  else if (voltage > 100)
  {
    return 100;
  }
  else
  {
    return int(voltage);
  }
}
