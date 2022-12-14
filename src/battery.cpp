/** Forked from https://github.com/pangodream/18650CL */

#include <Arduino.h>
#include "battery.h"

const double CONVERSION_FACTOR = 1.7;
const double NUM_READS = 20;

/**
 * Maps each percent (index) to a voltage value
 */
const double PERCENT_TO_VOLTS[] = {
    .200,
    .250,
    .300,
    .350,
    .400,
    .450,
    .500,
    .550,
    .600,
    .650,
    3.700,
    3.703,
    3.706,
    3.710,
    3.713,
    3.716,
    3.719,
    3.723,
    3.726,
    3.729,
    3.732,
    3.735,
    3.739,
    3.742,
    3.745,
    3.748,
    3.752,
    3.755,
    3.758,
    3.761,
    3.765,
    3.768,
    3.771,
    3.774,
    3.777,
    3.781,
    3.784,
    3.787,
    3.790,
    3.794,
    3.797,
    3.800,
    3.805,
    3.811,
    3.816,
    3.821,
    3.826,
    3.832,
    3.837,
    3.842,
    3.847,
    3.853,
    3.858,
    3.863,
    3.868,
    3.874,
    3.879,
    3.884,
    3.889,
    3.895,
    3.900,
    3.906,
    3.911,
    3.917,
    3.922,
    3.928,
    3.933,
    3.939,
    3.944,
    3.950,
    3.956,
    3.961,
    3.967,
    3.972,
    3.978,
    3.983,
    3.989,
    3.994,
    4.000,
    4.008,
    4.015,
    4.023,
    4.031,
    4.038,
    4.046,
    4.054,
    4.062,
    4.069,
    4.077,
    4.085,
    4.092,
    4.100,
    4.111,
    4.122,
    4.133,
    4.144,
    4.156,
    4.167,
    4.178,
    4.189,
    4.200,
};

int pinRead(uint8_t pin)
{
    int totalValue = 0;
    int averageValue = 0;
    for (int i = 0; i < NUM_READS; i++)
    {
        totalValue += analogRead(pin);
    }
    averageValue = totalValue / NUM_READS;
    return averageValue;
}

double analogReadToVolts(uint16_t readValue)
{
    double volts;
    volts = readValue * CONVERSION_FACTOR / 1000;
    return volts;
}

double getBatteryVolts(uint8_t pin)
{
    uint16_t readValue = pinRead(pin);
    return analogReadToVolts(readValue);
}

/**
 * Performs a binary search to find the index corresponding to a voltage.
 * The index of the array is the charge %
 */
int getChargeLevel(double volts)
{
    int idx = 50;
    int prev = 0;
    int half = 0;
    if (volts >= 4.2)
    {
        return 100;
    }
    if (volts <= 3.2)
    {
        return 0;
    }
    while (true)
    {
        half = abs(idx - prev) / 2;
        prev = idx;
        if (volts >= PERCENT_TO_VOLTS[idx])
        {
            idx = idx + half;
        }
        else
        {
            idx = idx - half;
        }
        if (prev == idx)
        {
            break;
        }
    }
    return idx;
}

int getBatteryChargeLevel(uint8_t pin)
{
    uint16_t readValue = pinRead(pin);
    double volts = analogReadToVolts(readValue);
    int chargeLevel = getChargeLevel(volts);

    return chargeLevel;
}
