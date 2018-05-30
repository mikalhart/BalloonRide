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
  log(F("Syntax is [ddd]C[xxx[,yyy]] where\r\n"));
  log(F("    ddd is an optional deferral in minutes (default=0)\r\n"));
  log(F("    C is the command to execute\r\n"));
  log(F("    xxx and yyy are arguments\r\n"));
  log(F("\r\n"));
  log(F(" CMD  Name                 Arguments\r\n"));
  log(F(" ---  -------------------- --------------------------\r\n"));
  log(F("  B   request Burst        duration(s) (opt, def=10s)\r\n"));
  log(F("  A   set target Altitude  target(m)   (opt, def=none)\r\n"));
  log(F("  M   execute Macro        macro#\r\n"));
  log(F("  P   take Picture         repeat interval(s) (opt, def=none, 0=stop)\r\n"));
  log(F("  V   take Video           duration(mins) (opt, def=infinite, 0=stop)\r\n"));
  log(F("  I   request Info packet  0=Primary, 1=Secondary\r\n"));
  log(F("  C   change Cadence       Arg1: 0=Gnd, 1=Flt, 2=Lnd, 3=sec\r\n"));
  log(F("                           Arg2: interval (min)\r\n"));
  log(F("\r\n"));
}

enum { STARTBURST, ENDBURST, TAKEPICTURE, STARTVIDEO, ENDVIDEO, MACRO };
struct SCHEDULEINFO
{
  time_t timestamp;
  int command;
  unsigned long arg;
};

#define TABLESIZE 8
struct SCHEDULEINFO events[TABLESIZE];
static const unsigned long ULONG_MAX = 0xFFFFFFFF;
static const unsigned long LONG_MAX = 0x7FFFFFFF;
static long target_altitude = LONG_MAX;

void processScheduler()
{
  time_t now = getMissionTime();
  for (int i=0; i<TABLESIZE; ++i)
  {
    // Event in the past?
    if (events[i].timestamp != 0 && now >= events[i].timestamp)
    {
      switch(events[i].command)
      {
        case STARTBURST:
          BurstStart(); break;
        case ENDBURST:
          BurstEnd(); break;
        case TAKEPICTURE:
          TakePicture(); break;
        case STARTVIDEO:
          VideoStart(); break;
        case ENDVIDEO:
          VideoEnd(); break;
        case MACRO:
          Macro(events[i].arg); break;
      }

      // Recurring pictures continue; everything else ceases
      if (events[i].command == TAKEPICTURE && events[i].arg != ULONG_MAX)
        events[i].timestamp += (int)events[i].arg;
      else
        events[i].timestamp = 0;
    }
  }
  
  if (target_altitude != LONG_MAX)
  {
    const GPSInfo &gpsinf = getGPSInfo();
    if (gpsinf.fixAcquired)
      MaintainAltitude(target_altitude, gpsinf.altitude);
  }
}

static void AddToScheduler(time_t timestamp, int command, unsigned long arg)
{
  int found = -1;
  for (int i=0; i<TABLESIZE; ++i)
  {
    if (found == -1 && events[i].timestamp == 0)
      found = i;
    if (events[i].command == command)
    {
      found = i;
      break;
    }
  }

  if (found != -1)
  {
    log("Found open scheduler slot at %d\r\n", found);
    events[found].timestamp = timestamp;
    events[found].command = command;
    events[found].arg = arg;
  }
}

static void RemoveFromScheduler(int command)
{
  for (int i=0; i<TABLESIZE; ++i)
    if (events[i].command == command)
      events[i].timestamp = 0;
}

static void Cadence(unsigned long arg1, unsigned long arg2)
{
  log(F("Processing Cadence command %lu %lu\r\n"), arg1, arg2);
  switch(arg1)
  {
    case 0:
      setGroundInterval(arg2);
      break;
    case 1:
      setFlightInterval(arg2);
      break;
    case 2:
      setPostLandingInterval(arg2);
      break;
    case 3:
      setSecondaryInterval(arg2);
      break;
  }
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

  else
  {
    errortok = tok1;
  }

  if (errortok)
  {
    return executeRemoteCommand(tok1);
  }

  else
  {
    return true;
  }
}

bool executeRemoteCommand(char *cmd)
{
  log("Executing remote command %s\r\n", cmd);
  char cpy[sizeof(IridiumInfo::receiveBuffer)];
  strncpy(cpy, cmd, sizeof(cpy));
  char *errortok = NULL;
  for (char *p = cpy; p != NULL && *p && errortok == NULL;)
  {
    char *tok = strsep(&p, ";");
    unsigned long defer = 0;
    char command;
    unsigned long arg1 = ULONG_MAX;
    unsigned long arg2 = ULONG_MAX;

    // Prefaced with "defer" value (minutes)?
    if (isdigit(*tok))
    {
      defer = strtoul(tok, &tok, 10);
    }
    command = *tok++;
    if (isdigit(*tok))
    {
      arg1 = (unsigned)strtoul(tok, &tok, 10);
    }

    if (*tok == ',' && isdigit(*++tok))
    {
      arg2 = (unsigned)strtoul(tok, &tok, 10);
    }

    time_t exectime = getMissionTime() + 60 * defer;
    log("Processing %c command\r\n", command);
    log("Defer = %lu\r\n", defer);
    log("Arg1 = %lu\r\n", arg1);
    log("Arg2 = %lu\r\n", arg2);
    log("Time = %lu\r\n", getMissionTime());
    log("ExecTime = %ld\r\n", exectime);
    
    switch(command)
    {
      case 'B':
        AddToScheduler(exectime, STARTBURST, 0);
        AddToScheduler(exectime + (arg1 == ULONG_MAX ? 10 : (unsigned)arg1), ENDBURST, 0);
        break;
      case 'A':
        target_altitude = arg1 == ULONG_MAX ? LONG_MAX : (long)arg1;
        break;
      case 'M':
        AddToScheduler(exectime, MACRO, arg1);
        break;
      case 'P':
        if (arg1 == 0) // 0 means stop taking pictures
          RemoveFromScheduler(TAKEPICTURE);
        else
          AddToScheduler(exectime, TAKEPICTURE, arg1);
        break;
      case 'V':
        if (arg1 == 0) // 0 means stop taking video
        {
          AddToScheduler(exectime, ENDVIDEO, 0);
        }
        else
        {
          AddToScheduler(exectime, STARTVIDEO, 0);
          if (arg1 != ULONG_MAX) // no parameter means record forever
            AddToScheduler(exectime + 60 * arg1, ENDVIDEO, 0);
        }
        break;
      case 'I':
        if (arg1 == 1)
          requestSecondaryInfo();
        else
          requestPrimaryInfo();
        break;
      case 'C':
        Cadence(arg1, arg2);
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
