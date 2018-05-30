#include <Arduino.h>
#include <TinyGPS++.h>  // NMEA parsing: http://arduiniana.org
#include "BalloonRide.h"
/*
 * Handle the Adafruit Ultimate GPS module attached to the interior
 */

static HardwareSerial &gps = GPSSerial;
static TinyGPSPlus tinyGps;
static struct GPSInfo info;

void gpsOn()
{
  pinMode(gpsPowerPin, INPUT);
}

void gpsOff()
{
  pinMode(gpsPowerPin, OUTPUT);
  digitalWrite(gpsPowerPin, LOW);
}

void startGPS()
{
  consoleText(F("Starting GPS..."));
  displayText(F("Starting GPS..."));
  gpsOn();
  gps.begin(gpsBaud);
  
  // turn off all but GGA and RMC for MTK3339 chip
  gps.print("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");

  // If no GPS characters detected in 5 seconds, this is a fatal fail
  int charsSeen = 0;
  bool newlineSeen = false;
  for (time_t now = getMissionTime(); charsSeen < 100 && getMissionTime() - now < 5;)
  {
    if (gps.available() > 0)
    {
      char c = gps.read();
      if (c == '\r' || c == '\n')
        newlineSeen = true;
      if (newlineSeen)
        consoleText(c);
      charsSeen++;
    }
  }
  
  if (charsSeen < 100)
    fatal(BALLOON_ERR_GPS_WIRING);
  consoleText("\r\nDone.\r\n");
  displayText("OK.\r\n");
}

void processGPS()
{
  // Read from GPS until (a) 1 second has elapsed or until a break of more than 100ms has occurred
  unsigned long start = millis();
  unsigned long timeOfLastChar = 0;
  unsigned long now = start;
  
  while ((timeOfLastChar == 0 || now - timeOfLastChar <= 100UL) && now - start <= 1000UL)
  {
    if (gps.available())
    {
      while (gps.available() > 0)
        tinyGps.encode(gps.read());
      timeOfLastChar = millis();
    }
    now = millis();
  }

  if (tinyGps.location.isUpdated() || tinyGps.date.isUpdated() || tinyGps.time.isUpdated())
  {
    info.latitude = tinyGps.location.lat();
    info.longitude = tinyGps.location.lng();
    info.year = tinyGps.date.year();
    info.month = tinyGps.date.month();
    info.day = tinyGps.date.day();
    info.hour = tinyGps.time.hour();
    info.minute = tinyGps.time.minute();
    info.second = tinyGps.time.second();
    info.hundredths = tinyGps.time.centisecond();
    info.altitude = tinyGps.altitude.meters();
    info.course = tinyGps.course.deg();
    info.speed = tinyGps.speed.knots();
    info.satellites = tinyGps.satellites.value();
  }

  info.fixAcquired = tinyGps.location.isValid() &&
    tinyGps.date.isValid() &&
    tinyGps.time.isValid() &&
    tinyGps.altitude.isValid();
  info.staleFix = info.fixAcquired && (tinyGps.location.age() > 2000 || tinyGps.date.age() > 2000);
  info.age = info.fixAcquired ? max(tinyGps.location.age(), tinyGps.date.age()) : (unsigned long)-1;
  info.checksumFail = tinyGps.failedChecksum();
}

const struct GPSInfo &getGPSInfo()
{
  return info;
}
