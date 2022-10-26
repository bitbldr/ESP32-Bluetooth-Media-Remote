#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include <functional>
#include "buttons.h"
#include <stack>
#include <map>
#include <vector>

// #define DEBUG(x)          \
//     do                    \
//     {                     \
//         Serial.printf(x); \
//     } while (0)

// #define DEBUG2(x, y)         \
//     do                       \
//     {                        \
//         Serial.printf(x, y); \
//     } while (0)

#define DEBUG(x)
#define DEBUG2(x, y)

using InterruptFn = std::function<void(void)>;

const unsigned long DEBOUNCE_THRESHOLD_MS = 100;
const unsigned long MULTI_CLICK_THRESHOLD_MS = 300;
const unsigned long PRESS_HOLD_THRESHOLD_MS = 1000;
const unsigned long EVENT_TIMEOUT = 2000;

class Handler
{
public:
    uint8_t pin;

    void (*onClickFn)();
    void (*onDoubleClickFn)();
    void (*onPressHoldFn)();

    Handler(uint8_t pin)
    {
        this->pin = pin;
    }

    void registerClickHandler(void (*cb)())
    {
        this->onClickFn = cb;
    }

    void registerDoubleClickHandler(void (*cb)())
    {
        this->onDoubleClickFn = cb;
    }

    void registerPressHoldHandler(void (*cb)())
    {
        this->onPressHoldFn = cb;
    }
};

class PendingEvent
{
public:
    unsigned long lastEvent_ms;
    uint8_t clickCount = 0;
    Handler *handler;

    PendingEvent(Handler *handler)
    {
        this->lastEvent_ms = millis();
        this->handler = handler;
    }
};

enum change_state
{
    rising,
    falling
};

class ChangeInterrupt
{
public:
    uint8_t pin;
    change_state state;

    ChangeInterrupt(uint8_t pin, change_state state)
    {
        this->pin = pin;
        this->state = state;
    }
};

std::map<uint8_t, Handler *> handlers;
std::map<uint8_t, unsigned long> debounceLocks;
std::stack<ChangeInterrupt *> changeInterrupts;
std::map<uint8_t, PendingEvent *> pendingEvents;

void processChangeInterrupt(ChangeInterrupt *interrupt)
{
    unsigned long now = millis();
    uint8_t pin = interrupt->pin;

    // if debounce lock was set from the latest interrupt, initialize it to now
    // TODO: refactor debounceLock to be member of handler
    if (debounceLocks[pin] == true)
    {
        debounceLocks[pin] = now;
    }

    Handler *h = handlers[pin];
    // TODO: use pendingEvents.count(pin) instead to check if key exists
    if (pendingEvents.find(pin) == pendingEvents.end())
    {
        if (interrupt->state == falling)
        {
            // pin changed from a high -> low, track new pending event
            PendingEvent *e = new PendingEvent(h);

            pendingEvents[pin] = e;
        }
    }
    else
    {
        // a pending event already exists for this pin, update it
        PendingEvent *p = pendingEvents[pin];
        p->lastEvent_ms = now;

        if (interrupt->state == rising)
        {
            // pin change is low -> high, track as another click
            p->clickCount++;
        }
    }
}

void clearPendingEvent(std::map<uint8_t, PendingEvent *>::iterator i, PendingEvent *e)
{
    delete e;
    pendingEvents.erase(i);
}

void cleanExpiredDebounceLocks()
{
    unsigned long now = millis();
    for (std::pair<const uint8_t, unsigned long> pair : debounceLocks)
    {
        unsigned long pin = pair.first;
        unsigned long lock = pair.second;
        if (lock && lock - now > DEBOUNCE_THRESHOLD_MS)
        {
            // clear the debounce lock
            debounceLocks[pin] = false;
        }
    }
}

void processPendingEvents()
{
    for (auto i = pendingEvents.begin(); i != pendingEvents.end(); ++i)
    {
        PendingEvent *e = (*i).second;

        unsigned long now = millis();
        unsigned long timeElapsed_ms = now - e->lastEvent_ms;
        Handler *h = e->handler;
        uint8_t pin = h->pin;
        int pinState = digitalRead(pin);

        if (h->onDoubleClickFn == NULL && h->onPressHoldFn == NULL)
        {
            // trigger single-click event
            h->onClickFn();

            clearPendingEvent(i, e);
        }
        else if (h->onDoubleClickFn != NULL && pinState == HIGH && timeElapsed_ms > MULTI_CLICK_THRESHOLD_MS)
        {
            if (e->clickCount > 1)
            {
                // trigger double-click event
                h->onDoubleClickFn();
            }
            else if (h->onClickFn != NULL)
            {
                // trigger single-click event
                h->onClickFn();
            }

            clearPendingEvent(i, e);
        }
        else if (h->onPressHoldFn != NULL && pinState == LOW && e->clickCount < 1 && timeElapsed_ms > PRESS_HOLD_THRESHOLD_MS)
        {
            // trigger press and hold event
            h->onPressHoldFn();

            clearPendingEvent(i, e);
        }
        else if (timeElapsed_ms > EVENT_TIMEOUT)
        {
            // the event has timed out and never completed for some reason
            // gracefully clear it without triggering anything
            clearPendingEvent(i, e);
        }
    }
}

void buttonEventLoop()
{
    // critical code - handler must complete and be cleaned up without an interrupt
    noInterrupts();

    while (!changeInterrupts.empty())
    {
        ChangeInterrupt *interrupt = changeInterrupts.top();
        processChangeInterrupt(interrupt);
        delete interrupt;
        changeInterrupts.pop();
    }

    processPendingEvents();
    cleanExpiredDebounceLocks();

    // re-enable interrupts
    interrupts();
}

void maybeInitializeHandler(uint8_t pin)
{
    // check if the pin is already assigned to a handler
    if (handlers.find(pin) == handlers.end())
    {
        // create a new handler for this pin
        handlers[pin] = new Handler(pin);

        // define an interrupt routine that will push a button change event to the stack
        InterruptFn btn_change_fn{
            [pin]()
            {
                if (!debounceLocks[pin])
                {
                    changeInterrupts.push(new ChangeInterrupt(pin, digitalRead(pin) == HIGH ? rising : falling));

                    debounceLocks[pin] = true;
                }
            }};

        attachInterrupt(digitalPinToInterrupt(pin), btn_change_fn, CHANGE);
    }
}

void onClick(uint8_t pin, void (*cb)())
{
    maybeInitializeHandler(pin);

    // assign click handler
    handlers[pin]->registerClickHandler(cb);
}

void onDoubleClick(uint8_t pin, void (*cb)())
{
    maybeInitializeHandler(pin);

    // assign double click handler
    handlers[pin]->registerDoubleClickHandler(cb);
}

void onPressHold(uint8_t pin, void (*cb)())
{
    maybeInitializeHandler(pin);

    // assign press and hold handler
    handlers[pin]->registerPressHoldHandler(cb);
}
