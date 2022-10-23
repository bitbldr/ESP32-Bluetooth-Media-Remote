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

std::map<uint8_t, Handler *> handlers;
std::map<uint8_t, unsigned long> debounceLocks;
std::stack<uint8_t> changeInterrupts;
std::map<uint8_t, PendingEvent *> pendingEvents;

void processChangeInterrupts(uint8_t pin)
{
    unsigned long now = millis();
    int pinState = digitalRead(pin);

    // if debounce lock was set from the latest interrupt, initialize it to now
    // TODO: refactor debounceLock to be member of handler
    if (debounceLocks[pin] == true)
    {
        debounceLocks[pin] = now;
    }

    Handler *h = handlers[pin];
    if (pendingEvents[pin] != NULL)
    {
        DEBUG2("DEBUG: pending event already exists for pin %d\n", pin);

        // a pending event already exists for this pin, update it
        PendingEvent *p = pendingEvents[pin];
        p->lastEvent_ms = now;

        if (pinState == HIGH)
        {
            // pin change is low -> high, track as another click
            p->clickCount++;
        }
    }
    else
    {
        if (pinState == LOW)
        {
            DEBUG2("DEBUG: create a new pending event for pin %d\n", pin);

            // pin changed from a high -> low, track new pending event
            PendingEvent *e = new PendingEvent(h);

            pendingEvents[pin] = e;

            DEBUG("DEBUG: add to pendingEvents\n");
        }
    }
}

void clearPendingEvent(uint8_t pendingEventKey)
{
    delete pendingEvents[pendingEventKey];
    pendingEvents.erase(pendingEventKey);
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
    std::stack<uint8_t> cleanupEvents;
    for (std::pair<const uint8_t, PendingEvent *> pair : pendingEvents)
    {
        uint8_t pendingEventKey = pair.first;

        DEBUG2("pendingEventKey %d\n", pendingEventKey);

        PendingEvent *e = pair.second;
        unsigned long now = millis();
        unsigned long timeElapsed_ms = now - e->lastEvent_ms;
        Handler *h = e->handler;
        uint8_t pin = h->pin;
        int pinState = digitalRead(pin);

        if (h->onPressHoldFn != NULL && e->clickCount < 1 && timeElapsed_ms > PRESS_HOLD_THRESHOLD_MS && pinState == LOW)
        {
            // trigger press and hold event
            h->onPressHoldFn();

            cleanupEvents.push(pendingEventKey);
        }
        else if (h->onDoubleClickFn != NULL && timeElapsed_ms > MULTI_CLICK_THRESHOLD_MS && pinState == HIGH)
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

            cleanupEvents.push(pendingEventKey);
        }
        else if (h->onPressHoldFn == NULL && h->onDoubleClickFn == NULL && pinState == HIGH)
        {
            // trigger single-click event
            h->onClickFn();

            cleanupEvents.push(pendingEventKey);
        }
        else if (timeElapsed_ms > EVENT_TIMEOUT)
        {
            // the event has timed out and never completed for some reason
            // gracefully clear it without triggering anything
            cleanupEvents.push(pendingEventKey);
        }
    }

    while (!cleanupEvents.empty())
    {
        uint8_t key = cleanupEvents.top();
        clearPendingEvent(key);
        cleanupEvents.pop();
    }
}

void buttonEventLoop()
{
    // critical code - handler must complete and be cleaned up without an interrupt
    noInterrupts();

    while (!changeInterrupts.empty())
    {
        uint8_t pin = changeInterrupts.top();
        processChangeInterrupts(pin);
        changeInterrupts.pop();
    }

    // re-enable interrupts
    interrupts();

    processPendingEvents();
    cleanExpiredDebounceLocks();
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
                    changeInterrupts.push(pin);
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
