#include <Arduino.h>
#include <IridiumSBD.h> // Satellite comms: http://arduiniana.org
#include <TinyGPS++.h>
#include <time.h>
#include "BalloonRide.h"

/*
   Handle communication with the RockBLOCK Iridium satellite modem
*/

static HardwareSerial &iridium = IridiumSerial;
// static IridiumSBD modem(iridium, rockBLOCKSleepPin, rockBLOCKRingPin); // We'll ignore RING for now
static IridiumSBD modem(iridium, rockBLOCKSleepPin, -1);
static struct IridiumInfo info;
static struct BalloonInfo bal_info;

static bool requestPrimary = false;
static bool requestSecondary = false;
static int  latestTxRxCode = ISBD_SUCCESS;

// Internal functions
static bool decideToTransmitPrimary();
static bool decideToTransmitSecondary();
typedef enum {PRIMARY, SECONDARY, ACK_RESPONSE, NAK_RESPONSE} PACKET_TYPE;
typedef enum {NONE, ACK, NAK} ACK_TYPE;
static bool txrx(const char *buf, const char *txtype, ACK_TYPE *pat);

void startIridium()
{
  log("Setting up satmodem...");
  displayText("Conf satmodem..");
  // First, start the serial port attached to the RockBLOCK
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  iridium.begin(rockBLOCKBaud);

  // ... and then the RockBLOCK itself
  modem.adjustATTimeout(90);
  int err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    log("modem.begin fail: %d\r\n", err);
    displayText("fail");
    fatal(BALLOON_ERR_IRIDIUM_INIT);
  }
  log("done.\r\n");
  displayText("OK.");
}


void processIridium()
{
  const GPSInfo &ginf = getGPSInfo();
  const BatteryInfo &binf = getBatteryInfo();
  const ThermalInfo &tinf = getThermalInfo();
  ACK_TYPE ackType = NONE;
  unsigned long now = millis();

  // Should we transmit a primary info packet?
  if (decideToTransmitPrimary())
  {
    // Create the buffer that will be sent to the RockBLOCK
    if (!ginf.fixAcquired)
    {
      snprintf(info.transmitBuffer1, sizeof(info.transmitBuffer1),
               "%d:no GPS,%.2f,%.2f",
               info.rxMessageNumber, binf.batteryVoltage, tinf.temperature[0]);
    }

    else
    {
      snprintf(info.transmitBuffer1, sizeof(info.transmitBuffer1),
               "%d:%02d%02d%02d,%.6f,%.6f,%ld,%.2f,%.2f",
               info.rxMessageNumber % 100, ginf.hour, ginf.minute, ginf.second,
               ginf.latitude, ginf.longitude, ginf.altitude, binf.batteryVoltage,
               tinf.temperature[0]);
    }

    if (txrx(info.transmitBuffer1, "Primary", &ackType))
    {
      info.xmitTime1 = now;
      info.alt = ginf.altitude;
      info.lat = ginf.latitude;
      info.lng = ginf.longitude;
      requestPrimary = false;
    }
  }

  // Should we transmit a secondary info packet?
  if (decideToTransmitSecondary())
  {
    // external temp, ballast, satellites, course, speed
    snprintf(info.transmitBuffer2, sizeof(info.transmitBuffer2),
             "S%d:%.2f,%d,%.2f,%.2f,0,0",
             info.rxMessageNumber, tinf.temperature[0],
             ginf.satellites, ginf.course, ginf.speed);
    if (txrx(info.transmitBuffer2, "Secondary", &ackType))
    {
      info.xmitTime2 = now;
      requestSecondary = false;
    }
  }

#if false // decided 1/18 not to send confusing ACK messages
  // Do we need to transmit an acknowledgement of received data?
  while (ackType != NONE)
  {
    const char *prefix = ackType == ACK ? "ACK" : "NAK";
    char buf[4 + sizeof(info.receiveBuffer) + 1];
    snprintf(buf, sizeof(buf), "%s:%s", prefix, info.receiveBuffer);
    while (!txrx(buf, ackType == ACK ? "ACK Acknowledgement" : "NAK Acknowledgement", &ackType));
  }
#endif
}

const IridiumInfo &getIridiumInfo()
{
  return info;
}

const BalloonInfo &getBalloonInfo()
{
  return bal_info;
}

void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  iridiumLog(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  log(c);
}

// returns true if it's time to transmit a primary info packet
static bool decideToTransmitPrimary()
{
  unsigned long now = millis();
  unsigned long secsSinceLastXmit = (now - info.xmitTime1) / 1000;

  bal_info.isDescending = false;

  bool mustTransmit = false;
  bool ringAsserted = modem.hasRingAsserted();
  const GPSInfo &ginf = getGPSInfo();

  // Don't transmit in the first 5 minutes unless we have a fix
  if (now < 5UL * 60UL * 1000UL && !ginf.fixAcquired)
    return false;

  // If we have a fix let's calculate a few useful things.
  if (ginf.fixAcquired)
  {
    // Ground altitude is the first thing we record
    if (ginf.satellites >= 5 && bal_info.groundAltitude == INVALID_ALTITUDE)
      bal_info.groundAltitude = ginf.altitude;

    // ... and the max altitude attained so far in this flight...
    if (bal_info.maxAltitude == INVALID_ALTITUDE || ginf.altitude > bal_info.maxAltitude)
      bal_info.maxAltitude = ginf.altitude;

    if (info.xmitTime1 != 0UL && info.lat != INVALID_LATLONG && info.lng != INVALID_LATLONG && info.alt != INVALID_ALTITUDE)
    {
      // ... and whether the balloon has moved significantly since the last transmission
      bal_info.lateralTravel = TinyGPSPlus::distanceBetween(ginf.latitude, ginf.longitude, info.lat, info.lng);
      bal_info.verticalTravel = abs(info.alt - ginf.altitude);

      // If it's moved since the most recent transmission, it's in flight
      if (bal_info.lateralTravel > 1000.0 || bal_info.verticalTravel > 100UL)
      {
        bal_info.flightState = BalloonInfo::INFLIGHT;
        bal_info.hasEverFlown = true;
      }
      // if it hasn't, it's either still on the ground or landed
      else if (secsSinceLastXmit > 10 && bal_info.hasEverFlown)
      {
        bal_info.flightState = BalloonInfo::LANDED;
      }
    }

    // ... and whether the balloon is descending
    bal_info.isDescending = bal_info.flightState == BalloonInfo::INFLIGHT && ginf.altitude < info.alt;
  }

  // Here are the cases when we should transmit a message on the sat modem:
  // 1. If the client requested it...
  if (requestPrimary)
  {
    log(F("Primary info requested by client..\r\n"));
    mustTransmit = true;
  }

  // 2. If we've never transmitted before...
  else if (info.xmitTime1 == 0UL)
  {
    log(F("Transmitting for first time.\r\n"));
    mustTransmit = true;
  }

  // 3. If the balloon is still on the ground and it's been more than GROUND_INTERVAL since our last transmission...
  else if (bal_info.maxAltitude < bal_info.groundAltitude + 1000L && secsSinceLastXmit >= info.GROUND_INTERVAL * 60UL)
  {
    log(F("Still close to ground: transmitting on %d-minute cadence.\r\n"), info.GROUND_INTERVAL);
    mustTransmit = true;
  }

  // 4. If the balloon is in flight and it's been more than FLIGHT_INTERVAL...
  else if (bal_info.flightState == BalloonInfo::INFLIGHT && secsSinceLastXmit >= info.FLIGHT_INTERVAL * 60UL) // time in seconds
  {
    log(F("Balloon in flight: transmitting on %d-minute cadence.\r\n"), info.FLIGHT_INTERVAL);
    mustTransmit = true;
  }

  // 5. If the balloon is descending and is close to the ground, transmit as frequently as possible.
  //    This probably indicates that the balloon is about to land.  If it does so upside-down or in water and cannot
  //    transmit after touching down, this strategy may help us find the payload.
  else if (bal_info.isDescending && ginf.altitude < bal_info.groundAltitude + 1000L)
  {
    log(F("Just about to land: transmitting as frequently as possible.\r\n"));
    mustTransmit = true;
  }

  // 5. If already landed and it's been a long since the last transmission, transmit even though the balloon isn't moving.
  else if (bal_info.flightState == BalloonInfo::LANDED && secsSinceLastXmit >= info.POST_LANDING_INTERVAL * 60UL)
  {
    log(F("It's been %d minutes: time to transmit.\r\n"), info.POST_LANDING_INTERVAL);
    mustTransmit = true;
  }

  // 6. If there are inbound messages waiting or RING has been asserted
  else if (ringAsserted || modem.getWaitingMessageCount() > 0)
  {
    if (ringAsserted)
      log(F("RING was asserted: transmit now.\r\n"));
      
    if (modem.getWaitingMessageCount() > 0)
      log(F("There are %d messages remaining: transmit now.\r\n"), modem.getWaitingMessageCount());
    mustTransmit = true;
  }

  return mustTransmit;
}

static bool decideToTransmitSecondary()
{
  unsigned long now = millis();
  unsigned long secsSinceLastXmit = (now - info.xmitTime2) / 1000;
  bool mustTransmit = false;

  // 1. If requested by client...
  if (requestSecondary)
  {
    log(F("Secondary info requested by client..\r\n"));
    mustTransmit = true;
  }

  // 2. If it's been a while (0 = never transmit)
  if (info.SECONDARY_INTERVAL != 0 && secsSinceLastXmit >= info.SECONDARY_INTERVAL * 60UL)
  {
    log(F("It's been %d minutes since last secondary: time to transmit.\r\n"), info.SECONDARY_INTERVAL);
    mustTransmit = true;
  }

  return mustTransmit;
}

static bool txrx(const char *buffer, const char *txtype, ACK_TYPE *pat)
{
  size_t rxBufSize = sizeof info.receiveBuffer - 1; // allow room for null terminator
  strcpy(info.receiveBuffer, ""); // clear rx buffer

  log(F("*****************************************\r\n"));
  log(F("Attempting RockBLOCK %s transmission..\r\n%s\r\n"), txtype, buffer);
  log(F("*****************************************\r\n"));

  /* New */
  if (modem.isAsleep())
  {
    log(F("Waking modem.\r\n"));
    int err = modem.begin();
    if (err != ISBD_SUCCESS)
    {
      log("modem.begin fail: %d\r\n", err);
      displayText("fail");
      fatal(BALLOON_ERR_IRIDIUM_INIT);
    }
    delay(2000); // needed??
  }
  
  /* New */
  
  info.isTransmitting = true;
  latestTxRxCode = modem.sendReceiveSBDText(buffer, reinterpret_cast<uint8_t *>(info.receiveBuffer), rxBufSize);
  info.isTransmitting = false;
  if (latestTxRxCode == ISBD_SUCCESS)
  {
    info.count++;
    log(F("TX succeeded.\r\n"));
    if (info.receiveBuffer[0])
    {
      info.receiveBuffer[rxBufSize] = '\0';
      log(F("Message received! \"%s\"\r\n"), info.receiveBuffer);
      ++info.rxMessageNumber;
      *pat = executeRemoteCommand(info.receiveBuffer) ? ACK : NAK;
      return true;
    }
    *pat = NONE;
  
    /* New */
    int err = modem.sleep();
    if (err != ISBD_SUCCESS)
    {
      log("modem.sleep fail: %d\r\n", err);
      displayText("fail");
      // fatal(BALLOON_ERR_IRIDIUM_INIT);
    }
    
    /* New */
    return true;
  }
  else
  {
    info.failcount++;
    log(F("TX failed: %d\r\n"), latestTxRxCode);
    return false;
  }
}

void setFlightInterval(uint16_t interval)
{
  info.FLIGHT_INTERVAL = interval;
  log("Flight interval set to %u\r\n", info.FLIGHT_INTERVAL);
}

void setGroundInterval(uint16_t interval)
{
  info.GROUND_INTERVAL = interval;
  log("Ground interval set to %u\r\n", info.GROUND_INTERVAL);
}

void setSecondaryInterval(uint16_t interval)
{
  info.SECONDARY_INTERVAL = interval;
  log("Secondary interval set to %u\r\n", info.SECONDARY_INTERVAL);
}

void setPostLandingInterval(uint16_t interval)
{
  info.POST_LANDING_INTERVAL = interval;
  log("Post-landing interval set to %u\r\n", info.POST_LANDING_INTERVAL);
}

void requestPrimaryInfo()
{
  requestPrimary = true;
}

void requestSecondaryInfo()
{
  requestSecondary = true;
}

bool Code3()
{
  return latestTxRxCode == 3;
}


