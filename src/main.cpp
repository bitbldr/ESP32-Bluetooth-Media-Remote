/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
#include <Arduino.h>
#include <cmath>
#include "buttons.h"
#include "battery.h"

#include <BleKeyboard.h>

#define DEBUG(x)      \
  do                  \
  {                   \
    Serial.printf(x); \
  } while (0)

#define DEBUG2(x, y)     \
  do                     \
  {                      \
    Serial.printf(x, y); \
  } while (0)

// #define DEBUG(x)
// #define DEBUG2(x, y)

BleKeyboard bleKeyboard("Blue Button", "bitbldr", 100);

uint8_t PWR_LED = 13;
uint8_t PLAY_PAUSE = 15;
uint8_t VOL_UP = 18;
uint8_t VOL_DOWN = 19;
uint8_t VBAT_SENSE = 35;

unsigned long lastEvent;
boolean isConnected = false;
unsigned long lastBatteryLevelUpdate = 0;

RTC_DATA_ATTR int clickCount = 0;
RTC_DATA_ATTR int dblClickCount = 0;
RTC_DATA_ATTR int pressHoldCount = 0;

// auto sleep after 5 minutes of inactivity
const unsigned long AUTO_SLEEP_INACTIVITY_TIMEOUT = 5 * 60 * 1000;
const bool ENABLE_DEEP_SLEEP = true;

// update battery level every 5 mins
const unsigned long BATTERY_UPDATE_INTERVAL_MS = 5 * 60 * 1000;

void ledAnimateFadeOff()
{
  // Fade out
  for (int ledVal = 255; ledVal >= 0; ledVal -= 1)
  {
    analogWrite(PWR_LED, ledVal);
    delay(2);
  }
}

void ledAnimateFadeOn()
{
  // Fade in
  for (int ledVal = 0; ledVal <= 255; ledVal += 1)
  {
    analogWrite(PWR_LED, ledVal);
    delay(2);
  }
}

void goToSleep()
{
  DEBUG("Going to sleep now\n");

  ledAnimateFadeOff();

  // allow 3 seconds to depress button so that sleep isn't immediately exited
  delay(3000);

  if (ENABLE_DEEP_SLEEP)
  {
    esp_deep_sleep_start();
  }
  else
  {
    esp_light_sleep_start();
  }
}

// Function that prints the reason by which ESP32 has been awaken from sleep
esp_sleep_wakeup_cause_t getWakeupReason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
  case 1:
    DEBUG("Wakeup caused by external signal using RTC_IO\n");
    break;
  case 2:
    DEBUG("Wakeup caused by external signal using RTC_CNTL\n");
    break;
  case 3:
    DEBUG("Wakeup caused by timer\n");
    break;
  case 4:
    DEBUG("Wakeup caused by touchpad\n");
    break;
  case 5:
    DEBUG("Wakeup caused by ULP program\n");
    break;
  default:
    DEBUG("Wakeup was not caused by deep sleep\n");
    break;
  }

  return wakeup_reason;
}

void onPlayPauseClick()
{
  DEBUG2("Play/Pause clicked %d times!\n", ++clickCount);

  if (bleKeyboard.isConnected())
  {
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
  }

  lastEvent = millis();
}

void onPlayPauseOnMultiClick(uint8_t clickCount)
{
  if (clickCount == 2)
  {
    DEBUG2("Play/Pause double-clicked %d times!\n", ++dblClickCount);

    if (bleKeyboard.isConnected())
    {
      bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    }
  }
  else if (clickCount == 3)
  {
    DEBUG2("Play/Pause triple-clicked %d times!\n", ++dblClickCount);

    if (bleKeyboard.isConnected())
    {
      bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    }
  }
  else
  {
    DEBUG2("Play/Pause multi-clicked %d times!\n", ++dblClickCount);
  }

  lastEvent = millis();
}

void onPlayPausePressHold()
{
  DEBUG2("Play/Pause press and hold %d times!\n", ++pressHoldCount);

  goToSleep();
}

void onVolUpClick()
{
  DEBUG("Vol +\n");

  if (bleKeyboard.isConnected())
  {
    bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
  }

  lastEvent = millis();
}

void onVolDownClick()
{
  DEBUG("Vol -\n");

  if (bleKeyboard.isConnected())
  {
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
  }

  lastEvent = millis();
}

void setup()
{
  Serial.begin(115200);

  pinMode(PWR_LED, OUTPUT);
  pinMode(PLAY_PAUSE, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  // check the wakeup reason for ESP32
  if (getWakeupReason() == 2)
  {
    // wakeup from deep sleep was caused by RTC_CNTL play/pause button
    // require a presshold for 3 seconds to ensure wakeup is not accidental
    unsigned long UNLOCK_THRESHOLD = 3 * 1000;
    unsigned long start = millis();
    unsigned long now = start;

    while (digitalRead(PLAY_PAUSE) == LOW && now - start < UNLOCK_THRESHOLD)
    {
      if (millis() % 1000 < 500)
      {
        analogWrite(PWR_LED, 0);
      }
      else
      {
        analogWrite(PWR_LED, 255);
      }

      now = millis();
    }

    if (digitalRead(PLAY_PAUSE) == HIGH)
    {
      // unlock threshold not met, go back to sleep
      DEBUG("Unlock threshold not met. Going back to sleep\n");
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);
      esp_deep_sleep_start();

      return;
    }
  }

  DEBUG("Starting BLE!\n");
  bleKeyboard.begin();

  onClick(PLAY_PAUSE, onPlayPauseClick);
  onMultiClick(PLAY_PAUSE, onPlayPauseOnMultiClick);
  onPressHold(PLAY_PAUSE, onPlayPausePressHold);
  onClick(VOL_UP, onVolUpClick);
  onClick(VOL_DOWN, onVolDownClick);

  // allow button press to wake up the controller
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);

  lastEvent = millis();

  ledAnimateFadeOn();
}

void discoverableLoop(unsigned long now)
{
  if (now % 200 < 100)
  {
    analogWrite(PWR_LED, 0);
  }
  else
  {
    analogWrite(PWR_LED, 255);
  }
}

void updateBatteryLevelLoop(unsigned long now)
{
  if (now - lastBatteryLevelUpdate > BATTERY_UPDATE_INTERVAL_MS)
  {
    bleKeyboard.setBatteryLevel(getBatteryChargeLevel(VBAT_SENSE));
    lastBatteryLevelUpdate = now;
  }
}

void onConnect()
{
  analogWrite(PWR_LED, 255);
  bleKeyboard.setBatteryLevel(getBatteryChargeLevel(VBAT_SENSE));
}

void connectedLoop(unsigned long now)
{
  if (!isConnected)
  {
    isConnected = true;
    onConnect();
  }

  updateBatteryLevelLoop(now);
}

void loop()
{
  unsigned long now = millis();
  if (now - lastEvent > AUTO_SLEEP_INACTIVITY_TIMEOUT)
  {
    goToSleep();
  }

  bleKeyboard.isConnected() ? connectedLoop(now) : discoverableLoop(now);

  buttonEventLoop();
}
