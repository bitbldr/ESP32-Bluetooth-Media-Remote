#pragma once

constexpr uint16_t SHUTTER_RELEASED = 0x0601;
constexpr uint16_t PRESS_TO_FOCUS = 0x0701;
constexpr uint16_t HOLD_FOCUS = 0x0801;
constexpr uint16_t TAKE_PICTURE = 0x0901;

void connectToSony();
void sonyCameraLoop();
bool sonyIsConnected();
bool sendSony(uint16_t value);
