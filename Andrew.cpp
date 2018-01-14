#include <Arduino.h>
#include "BalloonRide.h"

/*
 * Custom commands made (mostly) by Andrew :)
 */

int SHUTTER_CONTROL = 30;
int MODE_CONTROL = 31;
int PWR_CONTROL = 32;
boolean CameraPWRonORoff = false;

// Declare functions used later in the file
void PressPWR();
void PressSHUTTER();


// This is called once at startup
void AndrewsStartup()
{
  // All the initial "pinMode" stuff goes here
  log(F("Andrew's Startup\r\n"));
  pinMode(SHUTTER_CONTROL, INPUT);
  pinMode(MODE_CONTROL, OUTPUT);
  pinMode(PWR_CONTROL, INPUT);
}

void BurstStart()
{
  log(F("Burst Start\r\n"));
  /* TODO: maybe bring pin HIGH to burn filament? */
}

void BurstEnd()
{
  log(F("Burst End\r\n"));
  /* TODO: return pin to floating? */
}

void MaintainAltitude(long target, long current)
{
  log(F("Maintain Altitude target = %ld, current = %ld\r\n"), target, current);
  /* TODO */
}

void TakePicture()
{
  log(F("Take Picture\r\n"));
  /* TODO */
}

void VideoStart()
{
  log(F("Start Video\r\n"));
  if (CameraPWRonORoff == false)  PressPWR();
  PressSHUTTER();
  delay(2000);
}

void VideoEnd()
{
  log(F("End Video\r\n"));
  /* TODO */
}

void Macro(int n)
{
  log(F("Macro %d\r\n"), n);
  /* TODO */
}

void PressPWR()
{
  pinMode(PWR_CONTROL, OUTPUT);
  delay(2000);
  digitalWrite(PWR_CONTROL, HIGH);
  delay(4000);
  pinMode(PWR_CONTROL, INPUT);
  CameraPWRonORoff = !CameraPWRonORoff;

  delay(3000);
}

void PressSHUTTER()
{
  pinMode(SHUTTER_CONTROL, OUTPUT);
  digitalWrite(SHUTTER_CONTROL, LOW); // bring to ground
  delay(250); //
  // then return to floating:
  pinMode(SHUTTER_CONTROL, INPUT);
}

