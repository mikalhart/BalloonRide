#include <Arduino.h>
#include "BalloonRide.h"

/*
 * Handle the main battery and GPS backup battery
 */

static BatteryInfo info;
void startBatteryMonitor()
{
  info.batteryVoltage = INVALID_VOLTAGE;
  info.gpsBackupBatteryVoltage = INVALID_VOLTAGE;
}

void processBatteryData()
{
  static time_t lastTimeActive = 0; // only really do this once per second
  if (lastTimeActive == 0 || getMissionTime() != lastTimeActive)
  {
    lastTimeActive = getMissionTime();
    info.batteryVoltage = 2.0 * 3.3 * analogRead(mainBatteryVoltagePin) / 1024.0;
    info.gpsBackupBatteryVoltage = 5.0 * analogRead(gpsBackupBatteryVoltagePin) / 1024.0;
  }
}

const BatteryInfo &getBatteryInfo()
{
  return info;
}

