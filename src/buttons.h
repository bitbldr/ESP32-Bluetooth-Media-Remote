#include <Arduino.h>

void init();
void onClick(uint8_t pin, void (*cb)());
// void onMultiClick(uint8_t pin, void (*cb)());
// void onHold(uint8_t pin, unsigned long time, void (*cb)());
// void onComboHold(uint8_t *pins, unsigned long time, void (*cb)());