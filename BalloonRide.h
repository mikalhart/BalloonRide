#pragma once

/* Version 6.0 is designed for the Teensy 3.5 */
#if !defined(__MK64FX512__)
#error "This program requires Teensy 3.5"
#endif

typedef usb_serial_class ConsoleType;
typedef const __FlashStringHelper *FlashString;

// Port mappings
#define ConsoleSerial Serial
#define GPSSerial Serial2
#define IridiumSerial Serial3

// Constants
static const unsigned long gpsBaud = 9600UL;
static const unsigned long rockBLOCKBaud = 19200UL;
static const unsigned long consoleBaud = 115200UL;
static const int THERMAL_PROBES = 2;
#define VERSION "6.00"
static const double INVALID_VOLTAGE = -1000.0;
static const double INVALID_TEMPERATURE = -1000.0;
static const long INVALID_ALTITUDE = -20000L;
static const double INVALID_LATLONG = -1000.0;
typedef enum { LOG_IRIDIUM = 1, LOG_TELEMETRY = 2, LOG_RUNLOG = 4 } LOGTYPE;

// Pin assignments
// GPS connected to Serial2 (pins 9 and 10)
// RockBLOCK connected to Serial3 (pins 7 and 8)
static const int ledPin = 13;
static const int gpsFixPin = -1;
static const int gpsPPSPin = -1;
static const int gpsBackupBatteryVoltagePin = A1;
static const int mainBatteryVoltagePin = A0;
static const int rockBLOCKSleepPin = 4;
static const int rockBLOCKRingPin = 5;
static const int gpsPowerPin = -1;
static const int ds18B20pin0 = 11;
static const int ds18B20pin1 = 12;

// Error "blink" codes
static const int BALLOON_ERR_IRIDIUM_INIT = 2;
static const int BALLOON_ERR_SD_INIT = 3;
static const int BALLOON_ERR_LOG_FILE = 4;
static const int BALLOON_ERR_GPS_WIRING = 5;

// data structures
struct GPSInfo
{
   int year;
   byte month, day, hour, minute, second, hundredths;
   double latitude = INVALID_LATLONG, longitude = INVALID_LATLONG;
   long altitude = INVALID_ALTITUDE; // in meters
   double course, speed; // degrees, knots
   int satellites;
   bool fixAcquired;
   bool staleFix;
   long age;
   unsigned long checksumFail;
};

struct IridiumInfo
{
  // Default cadences for transmission of primary and secondary messages
  static const uint16_t DEFAULT_GROUND_INTERVAL = 3;        // primary message every 3 minutes while on ground
  static const uint16_t DEFAULT_FLIGHT_INTERVAL = 3;        // primary message every 3 minutes while in flight
  static const uint16_t DEFAULT_POST_LANDING_INTERVAL = 15; // primary message every 15 minutes after landing
  static const uint16_t DEFAULT_SECONDARY_INTERVAL = 0;     // never send secondary messages by default

  // Message cadences
  uint16_t GROUND_INTERVAL = DEFAULT_GROUND_INTERVAL;
  uint16_t FLIGHT_INTERVAL = DEFAULT_FLIGHT_INTERVAL;
  uint16_t POST_LANDING_INTERVAL = DEFAULT_POST_LANDING_INTERVAL;
  uint16_t SECONDARY_INTERVAL = DEFAULT_SECONDARY_INTERVAL;
  
  // Primary messages (location, altitude, battery, internal temperature)
  unsigned long xmitTime1;   // system time of last transmission
  double lat, lng;           // location at last transmission
  long alt;                  // altitude at last transmission
  char transmitBuffer1[128]; // most recent primary transmit string

  // Secondary messages (ballast, speed, course, satellites, external temperature)
  unsigned long xmitTime2;   // system time of last transmission
  char transmitBuffer2[128]; // most recent secondary transmit string

  // RX and general
  unsigned long count;           // number of successful transmissions
  unsigned long failcount;       // number of unsuccessful transmissions
  bool isTransmitting;           // true if in the middle of transmission
  char receiveBuffer[128];       // most recent receive buffer
  int rxMessageNumber = 0;       // count of successful receptions
};

struct BalloonInfo
{
  bool inFlight;                 // true if balloon is in flight
  bool isDescending;             // true if balloon descending
  double lateralTravel;          // meters traveled since last transmit
  unsigned long verticalTravel;  // meters traveled vertically since last transmit
  long groundAltitude = INVALID_ALTITUDE;
  long maxAltitude = INVALID_ALTITUDE;
};

struct ThermalInfo
{
   double temperature[THERMAL_PROBES];
};

struct BatteryInfo
{
   double batteryVoltage;
   double gpsBackupBatteryVoltage;
};

// prototypes
/* Battery */
extern void startBatteryMonitor();
extern void processBatteryData();
extern const BatteryInfo &getBatteryInfo();

/* Commands */
extern bool executeConsoleCommand(char *cmd);
extern bool executeRemoteCommand(char *cmd);
extern void showCommands();

/* Console */
extern void startConsole();
extern void processConsole();
extern void consoleText(const char *str);
extern void consoleText(int n);
extern void consoleText(char c);
extern void consoleText(FlashString fs);
extern uint8_t getConsoleViewFlags();
extern void setConsoleViewFlags(uint8_t flags);

/* Display */
extern void startDisplay();
extern void processDisplay();
extern void displayText(const char *str);
extern void displayText(int n);
extern void displayText(FlashString fs);

/* GPS */
extern void startGPS();
extern void processGPS();
extern const GPSInfo &getGPSInfo();

/* Iridium */
extern void startIridium();
extern void processIridium();
extern const IridiumInfo &getIridiumInfo();
extern const BalloonInfo &getBalloonInfo();
extern void setFlightInterval(uint16_t interval);
extern void setGroundInterval(uint16_t interval);
extern void setSecondaryInterval(uint16_t interval);
extern void setPostLandingInterval(uint16_t interval);
extern void requestPrimaryInfo();
extern void requestSecondaryInfo();

/* LED */
extern void startLED();
extern void processLED();
extern void blink(int count);

/* Logging */
extern void startLogs();
extern void processLogs();
extern void log(const char *fmt, ...);
extern void log(FlashString fmt, ...);
extern void log(char c);
extern void iridiumLog(char c);
extern void showLog(LOGTYPE whichLog);

/* Thermo */
extern void startThermalProbes();
extern void processThermalData();
extern const ThermalInfo &getThermalInfo();

/* Utilities */
extern uint32_t readEeprom32(int offset);
extern void writeEeprom32(uint32_t val, int offset);
extern void fatal(int code);


