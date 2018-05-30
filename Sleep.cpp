#include <Arduino.h>
#include "BalloonRide.h"
#include <Snooze.h>
/*
 * Rationale:
 * 
 * There are three modes: NORMAL, ALERT, and NORWAY.
 * 
 * NORMAL is normal
 * ALERT is NORMAL plus listen for RINGs (be responsive to commands)
 * NORWAY is ultra-conserving: wake only occasionally
 */

enum {NORMAL, ALERT, NORWAY};
static int mode = NORMAL;
static unsigned long DEFSLEEP = 3000;
static time_t systemStartTime = 0;

// Load drivers
SnoozeCompare compare;
SnoozeTimer timer;
SnoozeBlock config(timer/*, compare*/);

void startClocks()
{
  extern void *__rtc_localtime;
  rtc_set((uint32_t)&__rtc_localtime);
  systemStartTime = Teensy3Clock.get();
}

void startSleep()
{
  consoleText("In startSleep\r\n");
  timer.setTimer(DEFSLEEP);
  compare.pinMode(11, LOW, 1.65);//pin, type, threshold(v)
}

void processSleep()
{
  consoleText("In processSleep\r\n");
  delay(100);
  if (!getIridiumInfo().isTransmitting)
  {
    digitalWrite(ledPin, LOW);
    consoleText("About to sleep\r\n");
    delay(200);
    gpsOff();
    int who = Snooze.sleep(config);
    gpsOn();
    consoleText("Sleep 'who' is ");
    consoleText(who);
    consoleText("\r\n");
    digitalWrite(ledPin, HIGH);
  }
}

time_t getMissionTime()
{
  return Teensy3Clock.get() - systemStartTime;
}

