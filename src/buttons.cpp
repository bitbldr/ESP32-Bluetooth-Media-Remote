#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include <functional>
#include "buttons.h"
#include <stack>
#include <map>

using InterruptFn = std::function<void(void)>;

const unsigned long DEBOUNCE_MS = 50;

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

enum ButtonEventType
{
    btn_change,
    error
};

class ButtonEvent
{
public:
    uint8_t pin;
    ButtonEventType event;

    ButtonEvent(uint8_t pin, ButtonEventType event)
    {
        this->pin = pin;
        this->event = event;
    }
};

std::map<uint8_t, Handler *> handlers;
std::stack<ButtonEvent *> eventStack;

void process_event(ButtonEvent e)
{
    if (handlers[e.pin] != NULL)
    {
        // Handler *h = handlers[e.pin];
        Handler *h = handlers.at(e.pin);
        unsigned long now = millis();

        // if the time between this event and the last is within the debounce time, ignore it
        if ((now - h->lastEvent_ms) < DEBOUNCE_MS)
        {
            return;
        }

        if (digitalRead(e.pin) == HIGH)
        {
            h->cb();
        }

        h->lastEvent_ms = now;
    }
    else
    {
        Serial.printf("Error: No handler registered for pin %d\n", e.pin);
    }
}

void buttonEventLoop()
{
    while (!eventStack.empty())
    {
        // critical code - handler must complete and be cleaned up without an interrupt
        noInterrupts();

        ButtonEvent *e = eventStack.top();
        process_event(*e);
        eventStack.pop();
        delete e;

        interrupts();
    }
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
            eventStack.push(new ButtonEvent(pin, btn_change));
        }};

    attachInterrupt(digitalPinToInterrupt(pin), btn_change_fn, CHANGE);
}
