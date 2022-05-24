#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include <functional>
#include "buttons.h"
#include <stack>
#include <map>

using InterruptFn = std::function<void(void)>;

const unsigned long DEBOUNCE_MS = 100;
unsigned long lastDebounceEvent_ms = 0;

class Handler
{
public:
    unsigned long lastEvent_ms;
    uint8_t pin;
    void (*cb)();

    Handler(uint8_t pin, void (*cb)())
    {
        this->lastEvent_ms = millis();
        this->pin = pin;
        this->cb = cb;
    }
};

std::map<uint8_t, Handler *> handlers;
std::stack<uint8_t> eventStack;

void process_event(uint8_t pin)
{
    if (handlers[pin] != NULL)
    {
        unsigned long now = millis();
        Handler *h = handlers.at(pin);

        if (digitalRead(pin) == HIGH)
        {
            h->cb();
        }

        h->lastEvent_ms = now;
    }
    else
    {
        Serial.printf("Error: No handler registered for pin %d\n", pin);
    }
}

void buttonEventLoop()
{
    // critical code - handler must complete and be cleaned up without an interrupt
    noInterrupts();

    while (!eventStack.empty())
    {
        uint8_t pin = eventStack.top();
        process_event(pin);
        eventStack.pop();
    }

    // reenable interrupts
    interrupts();
}

void onClick(uint8_t pin, void (*cb)())
{
    // handle the error condition when pin is already assigned to a handler
    if (handlers.find(pin) != handlers.end())
    {
        Serial.printf("Error: Pin %d is already assigned to an event handler", pin);
        return;
    }

    // attach the callback to a new handler for this pin
    handlers[pin] = new Handler(pin, cb);

    InterruptFn btn_change_fn{
        [pin]()
        {
            unsigned long now = millis();
            if ((now - lastDebounceEvent_ms) < DEBOUNCE_MS)
            {
                return;
            }

            eventStack.push(pin);
            lastDebounceEvent_ms = now;
        }};

    attachInterrupt(digitalPinToInterrupt(pin), btn_change_fn, CHANGE);
}
