#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include <functional>
#include "buttons.h"
#include <stack>
#include <unordered_map>
#include <vector>

using InterruptFn = std::function<void(void)>;

const unsigned long DEBOUNCE_THRESHOLD_MS = 100;
const unsigned long MULTI_CLICK_THRESHOLD_MS = 300;
const unsigned long PRESS_HOLD_THRESHOLD_MS = 1000;
const unsigned long EVENT_TIMEOUT = 2000;

class Handler
{
public:
    uint8_t pin;
    unsigned long debounceLock;

    void (*onClickFn)();
    void (*onMultiClick)(uint8_t clickCount);
    void (*onPressHoldFn)();

    Handler(uint8_t pin)
    {
        this->pin = pin;
        this->onClickFn = NULL;
        this->onMultiClick = NULL;
        this->onPressHoldFn = NULL;
    }

    void registerClickHandler(void (*cb)())
    {
        this->onClickFn = cb;
    }

    void registeMultiClickHandler(void (*cb)(uint8_t clickCount))
    {
        this->onMultiClick = cb;
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

std::unordered_map<uint8_t, Handler *> handlers;
std::stack<ChangeInterrupt *> changeInterrupts;
std::unordered_map<uint8_t, PendingEvent *> pendingEvents;

void processChangeInterrupt(ChangeInterrupt *interrupt)
{
    unsigned long now = millis();
    uint8_t pin = interrupt->pin;
    Handler *h = handlers[pin];

    // if debounce lock was set from the latest interrupt, initialize it to now
    if (h->debounceLock == true)
    {
        h->debounceLock = now;
    }

    // TODO: use pendingEvents.count(pin) instead to check if key exists
    if (pendingEvents.count(pin) == 0)
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

void clearPendingEvent(std::unordered_map<uint8_t, PendingEvent *>::iterator i, PendingEvent *e)
{
    delete e;
    pendingEvents.erase(i);
}

void cleanExpiredDebounceLocks()
{
    unsigned long now = millis();
    for (std::pair<const uint8_t, Handler *> pair : handlers)
    {
        unsigned long lock = pair.second->debounceLock;
        if (lock && lock - now > DEBOUNCE_THRESHOLD_MS)
        {
            // clear the debounce lock
            pair.second->debounceLock = false;
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

        if (h->onMultiClick == NULL && h->onPressHoldFn == NULL && pinState == HIGH)
        {
            // trigger single-click event
            h->onClickFn();

            clearPendingEvent(i, e);
        }
        else if (h->onMultiClick != NULL && pinState == HIGH && timeElapsed_ms > MULTI_CLICK_THRESHOLD_MS)
        {
            if (e->clickCount > 1)
            {
                // trigger double-click event
                h->onMultiClick(e->clickCount);
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
    if (handlers.count(pin) == 0)
    {
        // create a new handler for this pin
        handlers[pin] = new Handler(pin);

        // define an interrupt routine that will push a button change event to the stack
        InterruptFn btn_change_fn{
            [pin]()
            {
                Handler *h = handlers.at(pin);
                if (!h->debounceLock)
                {
                    changeInterrupts.push(new ChangeInterrupt(pin, digitalRead(pin) == HIGH ? rising : falling));

                    h->debounceLock = true;
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

void onMultiClick(uint8_t pin, void (*cb)(uint8_t clickCount))
{
    maybeInitializeHandler(pin);

    // assign double click handler
    handlers[pin]->registeMultiClickHandler(cb);
}

void onPressHold(uint8_t pin, void (*cb)())
{
    maybeInitializeHandler(pin);

    // assign press and hold handler
    handlers[pin]->registerPressHoldHandler(cb);
}
