/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
#include <Arduino.h>
#include "buttons.h"

uint8_t PWR_LED = 13;
uint8_t PLAY_PAUSE = 15;
uint8_t VOL_UP = 18;
uint8_t VOL_DOWN = 19;

RTC_DATA_ATTR int clickCount = 0;
RTC_DATA_ATTR int dblClickCount = 0;
RTC_DATA_ATTR int pressHoldCount = 0;

void onPlayPauseClick()
{
  Serial.printf("Play/Pause clicked %d times!\n", ++clickCount);
}

void onPlayPauseonMultiClick(uint8_t clickCount)
{
  if (clickCount == 2)
  {
    Serial.printf("Play/Pause double-clicked %d times!\n", ++dblClickCount);
  }
  else if (clickCount == 3)
  {
    Serial.printf("Play/Pause triple-clicked %d times!\n", ++dblClickCount);
  }
  else
  {
    Serial.printf("Play/Pause multi-clicked %d times!\n", ++dblClickCount);
  }
}

void onPlayPausePressHold()
{
  Serial.printf("Play/Pause press and hold %d times!\n", ++pressHoldCount);
}

void onVolUpClick()
{
  Serial.printf("Vol +\n");
}

void onVolDownClick()
{
  Serial.printf("Vol -\n");
}

void blinkLoop()
{
  unsigned long now = millis();
  if (now % 1000 < 500)
  {
    digitalWrite(PWR_LED, LOW);
  }
  else
  {
    digitalWrite(PWR_LED, HIGH);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(PWR_LED, OUTPUT);
  pinMode(PLAY_PAUSE, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  onClick(PLAY_PAUSE, onPlayPauseClick);
  onMultiClick(PLAY_PAUSE, onPlayPauseonMultiClick);
  onPressHold(PLAY_PAUSE, onPlayPausePressHold);
  onClick(VOL_UP, onVolUpClick);
  onClick(VOL_DOWN, onVolDownClick);
}

void loop()
{
  blinkLoop();
  buttonEventLoop();
}
