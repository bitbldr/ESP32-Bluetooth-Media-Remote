#include <Arduino.h>
// #include <LinkedList.h>
#include <FunctionalInterrupt.h>
#include <functional>
#include "buttons.h"

#define MAX_PINS 32

using InterruptFn = std::function<void(void)>;

class Handler
{
public:
    unsigned long trigger_ms;
    uint8_t pin;
    void (*cb)();

    Handler(uint8_t pin, void (*cb)())
    {
        this->trigger_ms = millis();
        this->pin = pin;
        this->cb = cb;
    }
};

// unsigned long triggers[32];
// void (*callbacks[32])();
Handler *handlers[MAX_PINS];

void init()
{
    for (int i = 0; i < MAX_PINS; i++)
    {
        handlers[i] = 0;
    }
}

// LinkedList<Handler*> handlers = LinkedList<Handler*>();

void btn_down(uint8_t pin, void (*cb)())
{
    Serial.println("btn_down");
    handlers[pin] = new Handler(pin, cb);
}

void btn_up(uint8_t pin)
{
    Serial.println("btn_up");
    if (handlers[pin] != 0)
    {
        Handler *h = handlers[pin];
        h->cb();

        handlers[pin] = 0;
    }
}

void onClick(uint8_t pin, void (*cb)())
{
    // TODO: handle the error condition when pin is outside of triggers bounds

    Serial.println("onClick");

    InterruptFn down_fn{
        [pin, cb]()
        {
            Serial.println("down_fn");
            btn_down(pin, cb);
        }};

    attachInterrupt(digitalPinToInterrupt(pin), down_fn, FALLING);

    InterruptFn up_fn{
        [pin]()
        {
            Serial.println("up_fn");
            btn_up(pin);
        }};

    attachInterrupt(digitalPinToInterrupt(pin), up_fn, RISING);
}
