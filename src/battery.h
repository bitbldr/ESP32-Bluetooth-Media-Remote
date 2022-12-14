#ifndef BATTERY_h
#define BATTERY_h

/*
 * Get the battery charge level (0-100)
 * @return The calculated battery charge level
 */
int getBatteryChargeLevel(uint8_t pin);
double getBatteryVolts(uint8_t pin);
int getAnalogPin(uint8_t pin);
int pinRead(uint8_t pin);
double getConvFactor(uint8_t pin);

#endif