#ifndef BUTTONS_h
#define BUTTONS_h

void buttonEventLoop();
void onClick(uint8_t pin, void (*cb)());
void onMultiClick(uint8_t pin, void (*cb)(uint8_t clickCount));
void onPressHold(uint8_t pin, void (*cb)());

// void onComboHold(uint8_t *pins, unsigned long time, void (*cb)());

#endif