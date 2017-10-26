#include <Arduino.h>
#include <Eeprom.h>
#include "BalloonRide.h"

/*
 * General utilities
 */

uint32_t readEeprom32(int offset)
{
  return
    (((uint32_t)EEPROM.read(offset + 0)) << 0) |
    (((uint32_t)EEPROM.read(offset + 1)) << 8) |
    (((uint32_t)EEPROM.read(offset + 2)) << 16) |
    (((uint32_t)EEPROM.read(offset + 3)) << 24);
}

void writeEeprom32(uint32_t val, int offset)
{
  EEPROM.write(offset + 0, val & 0xFF);
  EEPROM.write(offset + 1, (val >> 8) & 0xFF);
  EEPROM.write(offset + 2, (val >> 16) & 0xFF);
  EEPROM.write(offset + 3, (val >> 24) & 0xFF);
}

/*
 * Fatal error: does not return.
 */

void fatal(int code)
{
  struct
  {
    int code;
    const char *msg;
    const char *shortMsg;
  } errorTable[] = 
  {
    {BALLOON_ERR_IRIDIUM_INIT, "FATAL: IridiumSBD init failure", "Iridium fail"},
    {BALLOON_ERR_SD_INIT, "FATAL: SD Filesystem failure (no SD card?)", "SD fail"},
    {BALLOON_ERR_LOG_FILE, "FATAL: SD Log file creation failure", "Log file fail"},
    {BALLOON_ERR_GPS_WIRING, "FATAL: GPS Wiring error: no GPS detected", "GPS fail"}
  };
  int msgCount = sizeof(errorTable) / sizeof(*errorTable);
  const char *msg = "Error: Unknown error.";
  const char *shortMsg = "Unknown error.";
  
  int i;
  for (i=0; i<msgCount; ++i)
    if (errorTable[i].code == code)
    {
      msg = errorTable[i].msg;
      shortMsg = errorTable[i].shortMsg;
      break;
    }
    
  log(msg);
  displayText(shortMsg);
  
  // Do this forever...
  while (true)
  {
    blink(code);
    delay(1000);
  }
}


