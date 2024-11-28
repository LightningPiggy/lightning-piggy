String lastTime = "";

String getTimeFromNTP()
{
  // Later on, this could validate the timezone even more,
  // and show a clear error if there's something wrong with it.
  if (!isConfigured(timezone))
    return "";
  if (!isWifiConnected())
    return "NoNetwork";

#ifdef DEBUG
  lastTime = "W23:39";
  return "W23:39";
#endif
  String timeData = getEndpointData(timeServer, String(timeServerPath) + String(timezone), false);

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, timeData);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return "time error";
  }

  Serial.println("Extracting weekday and time from received data");

  String datetimeAsString = doc["datetime"];

  int dayOfWeek = doc["day_of_week"];
  String dayOfWeekAsString = getDayOfWeekString(dayOfWeek);
  String timeString = datetimeAsString.substring(datetimeAsString.indexOf("T") + 1, datetimeAsString.indexOf("T") + 6); // Extract only the time (hh:mm)

  lastTime = dayOfWeekAsString + " " + timeString;
  return lastTime;
}

String getLastTime()
{
  return lastTime;
}

// In alphabetical order
String getDayOfWeekString(int dayOfWeek)
{
  if (dayOfWeek < 0 || dayOfWeek > 6)
  {
    Serial.println("Invalid day of week: " + String(dayOfWeek));
    return "";
  }

  return ptWeekdays[dayOfWeek];
}
