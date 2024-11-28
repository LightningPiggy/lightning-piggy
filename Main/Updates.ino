#include "Constants.h"

String newVersion = ""; // used by the update checker

int lastChecked = NOT_SPECIFIED;

bool isUpdateAvailable()
{
  return (newVersion != "" && newVersion != currentVersion);
}

String getShortHardwareInfo()
{
  int displayToUse = getDisplayToUse();
  if (displayToUse == DISPLAY_TYPE_213DEPG)
  {
    return "2.13D";
  }
  else if (displayToUse == DISPLAY_TYPE_266DEPG)
  {
    return "2.66D";
  }
  else
  {
    return "UNKNOWN";
  }
}

String getLongHardwareInfo()
{
  // Since the unified builds, we only know which display it is, not really which hardware.
  // But to be backwards compatible with the update checker metrics, we assume
  // a 2.13DEPG display must be the LILYGOT5V213
  // and 2.66DEPG must be the LILYGOT5V266
  int displayToUse = getDisplayToUse();
  if (displayToUse == DISPLAY_TYPE_213DEPG)
  {
    return "LILYGOT5V213|DEPG0213BN";
  }
  else if (displayToUse == DISPLAY_TYPE_266DEPG)
  {
    return "LILYGOT5V266|DEPG0266BN";
  }
  else
  {
    return "UNKNOWN|UNKNOWN";
  }
}

String getShortVersion()
{
  return currentVersion;
}

String getFullVersion()
{
  const char compiletime[] = __DATE__ " " __TIME__;
  String compileTime(compiletime);
  return currentVersion + "|" + getLongHardwareInfo() + "|" + compileTime;
}
