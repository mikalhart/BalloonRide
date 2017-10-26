#include <Arduino.h>
#include <EEPROM.h>
#include <IridiumSBD.h> // Satellite comms: http://arduiniana.org
#include <TinyGPS++.h>  // NMEA parsing: http://arduiniana.org
#include <SdFat.h>      // FAT filesystem for SD cards: https://github.com/greiman/SdFat
#include <OneWire.h>    // for DS18B20 thermometer
#include <Wire.h>       // I2C bus management for OLED Display
#include <Adafruit_GFX.h> // Adafruit base graphics library
#include <Adafruit_SSD1306.h> // Adafruit SSD1306 library
#include "BalloonRide.h"

/* Version 5.1.1
   This code is designed to operate with the ICBC "Balloon Ride" board version 5.1. a.k.a. "5c".
   It is similar to the version that successfully flew on the September 2014 flight using the 
   Balloon Ride 5 board.  Differences: (1) adds 2 new additional DS18B20 thermometer modules for 
   measuring outside temperature and perhaps the temperature of the experiment chamber, (2) moves
   the internal DS18B20 pin from pin 0 (D0) to pin 10 (C0), and (3) adds support for an external
   128x64 Adafruit OLED display.

   Version 6.0.0
   Modularized code
   Using new rev 1.0 IridiumSBD library
   Designed for receiving commands from ground (Burst, Ascend, Descend, etc.)
   New triple log
   Serial console command interface
*/

// State variables
static bool IridiumReentrant = false;
static bool setupComplete = false;

void setup()
{
  // Set up the status LED
  startLED();
  
  // Start the console port
  startConsole();

  // Greeting
  log("BalloonRide " VERSION "\r\n");
  log("Copyright (C) 2015-7 International Circumnavigation Balloon Consortium\r\n");
  log("Onwards and Upwards and Around the World!\r\n");
  log("\r\n");
  showCommands();

  // Set up the OLED display
  startDisplay();

  // Open SD log files
  startLogs();

  // Start up the thermal probes
  startThermalProbes();

  // Set up the GPS port
  startGPS();

  // Set up the RockBLOCK's serial port...
  startIridium();

  // Battery
  startBatteryMonitor();
  
  // All done with initialization!
  setupComplete = true;
}

void loop()
{
  processGPS();
  processThermalData();
  processBatteryData();
  processLogs();
  if (!IridiumReentrant)
    processIridium();
  processLED();
  processDisplay();
  processConsole();
}

// Recursively call loop during lengthy Iridium operations!
bool ISBDCallback()
{
  if (setupComplete)
  {
    IridiumReentrant = true;
    loop();
    IridiumReentrant = false;
  }
  
  return true;
}


