/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
#include <Arduino.h>
#include "buttons.h"

#define USE_NIMBLE
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

BleKeyboard bleKeyboard;

uint8_t PWR_LED = 13;
uint8_t PLAY_PAUSE = 15;
uint8_t VOL_UP = 18;
uint8_t VOL_DOWN = 19;

RTC_DATA_ATTR int clickCount = 0;
RTC_DATA_ATTR int dblClickCount = 0;
RTC_DATA_ATTR int pressHoldCount = 0;

void goToSleep()
{
  DEBUG("Going to sleep now\n");
  digitalWrite(PWR_LED, LOW);
  delay(1000);
  esp_deep_sleep_start();
  // esp_light_sleep_start();
}

// Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason()
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
}

void onPlayPauseClick()
{
  DEBUG2("Play/Pause clicked %d times!\n", ++clickCount);

  if (bleKeyboard.isConnected())
  {
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
  }
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
}

void onVolDownClick()
{
  DEBUG("Vol -\n");

  if (bleKeyboard.isConnected())
  {
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
  }
}

void ledBlinkLoop()
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

void ledSolidLoop()
{
  digitalWrite(PWR_LED, HIGH);
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Print the wakeup reason for ESP32
  print_wakeup_reason();

  DEBUG("Starting BLE work!");
  bleKeyboard.setName("BT Media Remote");
  bleKeyboard.begin();

  pinMode(PWR_LED, OUTPUT);
  pinMode(PLAY_PAUSE, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  onClick(PLAY_PAUSE, onPlayPauseClick);
  onMultiClick(PLAY_PAUSE, onPlayPauseOnMultiClick);
  onPressHold(PLAY_PAUSE, onPlayPausePressHold);
  onClick(VOL_UP, onVolUpClick);
  onClick(VOL_DOWN, onVolDownClick);

  // allow button press to wake up the controller
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);
}

void loop()
{
  bleKeyboard.isConnected() ? ledSolidLoop() : ledBlinkLoop();

  buttonEventLoop();
}
