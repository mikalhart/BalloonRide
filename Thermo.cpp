#include <Arduino.h>
#include <OneWire.h>    // for DS18B20 thermometer
#include "BalloonRide.h"

/*
 * Handle the internal and external DS18B20 thermometer modules.
 */

static struct ThermalInfo info;
static OneWire ds[THERMAL_PROBES] = {OneWire(ds18B20pin0), OneWire(ds18B20pin1)};
static byte thermAddress[THERMAL_PROBES][8];
static void readProbeData();

void startThermalProbes()
{
  log(F("Starting thermal probes...\r\n"));
  displayText(F("Thermal: "));
  bool fail = false;
  for (int i=0; i<THERMAL_PROBES; ++i)
  {
    // Device not found?
    if (!ds[i].search(thermAddress[i]))
    {
      ds[i].reset_search();
      log("Address search failed for thermal probe %d\r\n", i);
      fail = true;
      continue;
    }

    // Invalid CRC?
    if (OneWire::crc8(thermAddress[i], 7) != thermAddress[i][7])
    {
      log("CRC failed for thermal probe %d\r\n", i);
      fail = true;
      continue;
    }
  
    // Device not recognized?
    if (thermAddress[i][0] != 0x10 && thermAddress[i][0] != 0x28) 
    {
      log("Unknown device for thermal probe %d\r\n", i);
      fail = true;
      continue;
    }
    
    log("Probe %d has address ", i);
    for (int j=0; j<8; ++j)
      log("%02X%c", thermAddress[i][j], j == 7 ? ' ' : ':');
    log(fail ? "Fail.\r\n" : "OK\r\n");
  }
  displayText(fail ? "Fail\r\n" : "OK\r\n");
}

void processThermalData()
{
  static unsigned long lastTimeActive = 0UL; // only really do this once per second
  if (lastTimeActive == 0UL || millis() - lastTimeActive >= 1000)
  {
    lastTimeActive = millis();
    readProbeData();
  }
}

static void readProbeData()
{
  byte data[12];
  
  for (int i=0; i<THERMAL_PROBES; ++i)
  {
    if (thermAddress[i][0] != 0x10 && thermAddress[i][0] != 0x28) 
    {
      info.temperature[i] = INVALID_TEMPERATURE;
      continue;
    }
    ds[i].reset();
    ds[i].select(thermAddress[i]);
    ds[i].write(0x44, 1); // start conversion
  
    ds[i].reset();
    ds[i].select(thermAddress[i]);  
    ds[i].write(0xBE); // Read Scratchpad
  
    for (int j=0; j<9; j++)
      data[j] = ds[i].read();
  
    ds[i].reset_search();
    
    // info.temperature[i] = (double)((data[1] << 8) | data[0]) / 16.0;
    int16_t w = (data[1] << 8) | data[0];
    info.temperature[i] = (double)w / 16.0;
  }
}

const ThermalInfo &getThermalInfo()
{
  return info;
}

