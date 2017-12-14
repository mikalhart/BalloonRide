#include <Arduino.h>
#include "BalloonRide.h"

/*
 * Handle commands arriving from the serial console or through the satellite modem
 */

void showCommands()
{
  log(F("Console commands:\r\n"));
  log(F("\r\n"));
  log(F("  WATCH [all|none|telemetry|iridium|runlog]\r\n"));
  log(F("  TYPE telemetry|iridium|runlog\r\n"));
  log(F("\r\n"));
  log(F("Remote commands:\r\n"));
  log(F("\r\n"));
  log(F("  B[xxx]         Burst/disengage for specified seconds\r\n"));
  log(F("  Axxx           Ascend to specific altitude (m)\r\n"));
  log(F("  Dxxx           Descend to specific altitude (m)\r\n"));
  log(F("  P              Request Primary info packet immediately\r\n"));
  log(F("  S              Request Secondary info packet immediately\r\n"));
  log(F("  C<G|F|L|S>xxx  Set ground/flight/landing/secondary packet cadence.\r\n"));
  log(F("  Vxxx[,xxx]     Capture video of specified duration and (optionally) repeat minutes\r\n"));
  log(F("  I[xxx]         Capture a still image now and at the specified interval (seconds)\r\n"));
  log(F("\r\n"));
}

static void Burst(char *p)
{
  log(F("Processing Burst command %s\r\n"), p);
  int parameter = 0;
  if (strlen(p) >= 2)
  {
    parameter = (int)strtoul(p + 1, NULL, 10);
  }
  
  // TODO
}

static void Ascend(char *p)
{
  log(F("Processing Ascend command %s\r\n"), p);
  // TODO
}

static void Descend(char *p)
{
  log(F("Processing Descend command %s\r\n"), p);
  // TODO
}

/*
 * Commands to change the cadence at which primary or secondary info packets are transmitted from the balloon to the ground
 */

static void Cadence(char *p)
{
  log(F("Processing Cadence command %s\r\n"), p);
  if (strlen(p) >= 3)
  {
    uint16_t interval = (uint16_t)strtoul(p + 2, NULL, 10);
    switch(p[1])
    {
      case 'G':
        setGroundInterval(interval);
        break;
      case 'F':
        setFlightInterval(interval);
        break;
      case 'L':
        setPostLandingInterval(interval);
        break;
      case 'S':
        setSecondaryInterval(interval);
        break;
    }
  }
}

static void Image(char *p)
{
  log(F("Processing Image command %s\r\n"), p);
  // TODO
}

static void Video(char *p)
{
  log(F("Processing Video command %s\r\n"), p);
  // TODO
}

/*
 * Handle commands arriving from the serial console (watch/type)
 */

bool executeConsoleCommand(char *cmd)
{
  log(F("Executing command: '%s'\r\n"), cmd);
  
  char *p = cmd;
  char *tok1 = strsep(&p, " ");
  char *tok2 = strsep(&p, " ");
  char *errortok = NULL;

  if (!stricmp(tok1, "watch"))
  {
    if (!stricmp(tok2, "telemetry"))
      setConsoleViewFlags(LOG_TELEMETRY);
    else if (!stricmp(tok2, "iridium"))
      setConsoleViewFlags(LOG_IRIDIUM);
    else if (!stricmp(tok2, "runlog"))
      setConsoleViewFlags(LOG_RUNLOG);
    else if (!stricmp(tok2, "none"))
      setConsoleViewFlags(0);
    else if (!stricmp(tok2, "all"))
      setConsoleViewFlags(LOG_TELEMETRY | LOG_IRIDIUM | LOG_RUNLOG);
    else if (tok2 && strlen(tok2) > 0)
      errortok = tok2;
    else // no argument
    {
      if (getConsoleViewFlags() == 0)
        log("NONE");
      if (getConsoleViewFlags() & LOG_TELEMETRY)
        log("TELEMETRY ");
      if (getConsoleViewFlags() & LOG_IRIDIUM)
        log("IRIDIUM ");
      if (getConsoleViewFlags() & LOG_RUNLOG)
        log("RUNLOG");
      log("\r\n");
      
    }
  }

  else if (!stricmp(tok1, "type"))
  {
    if (!stricmp(tok2, "telemetry"))
      showLog(LOG_TELEMETRY);
    else if (!stricmp(tok2, "iridium"))
      showLog(LOG_IRIDIUM);
    else if (!stricmp(tok2, "runlog"))
      showLog(LOG_RUNLOG);
    else
      errortok = tok2;
  }

  if (errortok)
  {
    log(F("Unknown console command: '%s'. Trying remote.\r\n"), errortok);
    return executeRemoteCommand(tok1);
  }

  else
  {
    return true;
  }
}

bool executeRemoteCommand(char *cmd)
{
  char cpy[sizeof(IridiumInfo::receiveBuffer)];
  strncpy(cpy, cmd, sizeof(cpy));
  char *errortok = NULL;
  for (char *p = cpy; p != NULL && *p && errortok == NULL;)
  {
    char *tok = strsep(&p, ";");
    switch(*tok)
    {
      case 'B':
        Burst(tok);
        break;
      case 'A':
        Ascend(tok);
        break;
      case 'D':
        Descend(tok);
        break;
      case 'P':
        log(F("Processing Primary command %s\r\n"), tok);
        requestPrimaryInfo();
        break;
      case 'S':
        log(F("Processing Secondary command %s\r\n"), tok);
        requestSecondaryInfo();
        break;
      case 'C':
        Cadence(tok);
        break;
      case 'V':
        Video(tok);
        break;
      case 'I':
        Image(tok);
        break;
      default:
        log(F("Unknown command '%s'\r\n"), tok);
        return false;
    }
  }

  if (errortok != NULL)
  {
    log(F("Command error: %s\r\n"), errortok);
    return false;
  }
  log(F("Command complete\r\n"));
  return true;
}
