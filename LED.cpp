#include <Arduino.h>
#include "BalloonRide.h"

/*
 * Handle the on-board diagnostic LED
 */

void blink(int count);

void startLED()
{
  pinMode(ledPin, OUTPUT);
  blink(2); // Hello, world!
}

// Blink the LED "count" times.
void blink(int count)
{
  for (int i=0; i<count; ++i)
  {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
}

#if false
void processLED()
{
  // transmitting? 50 on 50 off
  // initialized but no fix 100% on
  // initialized and fix? 1000 off 1000 on 
  
  unsigned on = 100, off = 0;
  
  if (getIridiumInfo().isTransmitting) 
    on = off = 50;
  else if (getGPSInfo().fixAcquired) 
    on = off = 1000;
  
  unsigned long now = millis();
  digitalWrite(ledPin, now % (on + off) < on ? HIGH : LOW);  
}

#else
void processLED()
{
  // transmitting? blinking 50/50
  // no fix? off
  // fix? on
  bool on;

  if (getIridiumInfo().isTransmitting)
  {
    on = (millis() / 1000) % 2 == 1;
  }
  else if (getGPSInfo().fixAcquired)
  {
    on = true;
  }
  else
  {
    on = false;
  }
  
  digitalWrite(ledPin, on ? HIGH : LOW);
}
#endif
