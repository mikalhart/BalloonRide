#include <Arduino.h>
#include "BalloonRide.h"

/*
 * Handle I/O to/from the Serial console
 */

static uint8_t consoleView = LOG_IRIDIUM | LOG_TELEMETRY | LOG_RUNLOG;

// The Serial console
static ConsoleType &console = ConsoleSerial;

void startConsole()
{
  console.begin(consoleBaud);
}

void processConsole()
{
  static char buf[80];
  static int index = 0;

  while (console.available())
  {
    char c = console.read();
    if (c == '\r' || c == '\n' || index == sizeof buf - 1)
    {
      buf[index] = 0; // null terminator
      if (index > 0)
      {
        executeConsoleCommand(buf);
      }
      index = 0;
    }
    else
    {
      buf[index++] = c;
    }
  }
}

uint8_t getConsoleViewFlags()
{
  return consoleView;
}

void setConsoleViewFlags(uint8_t flags)
{
  consoleView = flags;
}

void consoleText(const char *str)
{
  console.print(str);
}

void consoleText(int n)
{
  console.print(n);
}

void consoleText(char c)
{
  console.write(c);
}

void consoleText(FlashString fs)
{
  console.print(fs);
}

