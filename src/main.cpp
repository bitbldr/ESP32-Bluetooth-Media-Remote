/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
#include <Arduino.h>

// #define USE_NIMBLE
#include <BleKeyboard.h>

#include "buttons.h"

BleKeyboard bleKeyboard;

// uint8_t GPIO0 = 0;
// uint8_t PWR_ON = 14;

uint8_t PWR_LED = 13;
uint8_t PLAY_PAUSE = 15;
uint8_t VOL_UP = 18;
uint8_t VOL_DOWN = 19;

#define timeSeconds 1000
#define debounce_ms 500
#define debug_en true
unsigned long now = millis();
unsigned long playPauseTrigger = 0;
unsigned long volUpTrigger = 0;
unsigned long volDownTrigger = 0;

RTC_DATA_ATTR int bootCount = 0;

void debug(const char *msg)
{
  if (debug_en)
  {
    Serial.println(msg);
  }
}

void IRAM_ATTR playPauseInterrupt()
{
  playPauseTrigger = millis();
}

void IRAM_ATTR volUpInterrupt()
{
  volUpTrigger = millis();
}

void IRAM_ATTR volDownInterrupt()
{
  volDownTrigger = millis();
}

void maybeHandlePlayPause(unsigned long now)
{
  if (playPauseTrigger != 0 && playPauseTrigger - now > debounce_ms)
  {
    if (digitalRead(PLAY_PAUSE) == LOW)
    {
      // button hold, do nothing
    }
    else
    {
      // process the event

      debug("Play/Pause");

      digitalWrite(PWR_LED, HIGH);

      playPauseTrigger = 0;
    }
  }
}

void maybeHandleVolUp(unsigned long now)
{
  if (volUpTrigger != 0 && volUpTrigger - now > debounce_ms)
  {

    debug("Vol +");

    digitalWrite(PWR_LED, HIGH);

    volUpTrigger = 0;
  }
}

void maybeHandleVolDown(unsigned long now)
{
  if (volDownTrigger != 0 && volDownTrigger - now > debounce_ms)
  {
    debug("Vol -");

    digitalWrite(PWR_LED, HIGH);

    volDownTrigger = 0;
  }
}

void onPlayPauseClick()
{
  Serial.println("Play/Pause clicked!");
}

void setup()
{
  Serial.begin(115200);

  // Serial.println("Starting BLE work!");
  // bleKeyboard.setName("BT Media Remote");
  // bleKeyboard.begin();

  Serial.println("Boot count: " + String(bootCount));
  bootCount++;

  pinMode(PWR_LED, OUTPUT);
  digitalWrite(PWR_LED, LOW);

  pinMode(PLAY_PAUSE, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  // pinMode(PWR_ON, OUTPUT);
  // pinMode(GPIO0, OUTPUT);
  // digitalWrite(GPIO0, LOW);

  // digitalWrite(PWR_ON, LOW);
  // delay(500);

  // attachInterrupt(digitalPinToInterrupt(PLAY_PAUSE), playPauseInterrupt, RISING);
  // attachInterrupt(digitalPinToInterrupt(VOL_UP), volUpInterrupt, RISING);
  // attachInterrupt(digitalPinToInterrupt(VOL_DOWN), volDownInterrupt, RISING);

  init();
  onClick(PLAY_PAUSE, onPlayPauseClick);

  // // allow button press to wake up the controller
  // esp_sleep_enable_gpio_wakeup();

  // if (bootCount < 1)
  // {
  //   Serial.println("Going to sleep now");
  //   esp_deep_sleep_start();
  // }
}

void loop()
{
  now = millis();

  // maybeHandlePlayPause(now);
  // maybeHandleVolUp(now);
  // maybeHandleVolDown(now);

  // // Turn off the LED after the number of seconds defined in the timeSeconds variable
  // if (startTimer && (now - lastTrigger > (1 * timeSeconds)))
  // {
  //   digitalWrite(PWR_LED, LOW);
  //   startTimer = false;
  //   // Serial.println("Going to sleep now");
  //   // esp_deep_sleep_start();
  // }

  // if (bleKeyboard.isConnected())
  // {
  //   if (digitalRead(PLAY_PAUSE) == LOW)
  //   {
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);

  //     delay(1000);
  //   }
  //   if (digitalRead(NEXT_TRACK) == LOW)
  //   {
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);

  //     delay(1000);
  //   }

  //   digitalWrite(LED_BUILTIN, LOW);

  //   // ---------------------
  //   // Serial.println("Sending 'Hello world'...");
  //   // bleKeyboard.print("Hello world");

  //   // delay(1000);

  //   // Serial.println("Sending Enter key...");
  //   // bleKeyboard.write(KEY_RETURN);

  //   // delay(1000);

  //   // Serial.println("Sending Play/Pause media key...");
  //   // bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);

  //   // delay(1000);

  //   //
  //   // Below is an example of pressing multiple keyboard modifiers
  //   // which by default is commented out.
  //   //
  //   /* Serial.println("Sending Ctrl+Alt+Delete...");
  //   bleKeyboard.press(KEY_LEFT_CTRL);
  //   bleKeyboard.press(KEY_LEFT_ALT);
  //   bleKeyboard.press(KEY_DELETE);
  //   delay(100);
  //   bleKeyboard.releaseAll();
  //   */
  // }
  // Serial.println("Waiting 5 seconds...");
  // delay(5000);
}
