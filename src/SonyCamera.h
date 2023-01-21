#pragma once

#include <NimBLEDevice.h>

constexpr std::array<uint8_t, 4> CAMERA_MANUFACTURER_LOOKUP = {0x2D, 0x01, 0x03, 0x00};
const std::__cxx11::string REMOTE_CONTROL_SERVICE_UUID = "8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF";

constexpr uint16_t SHUTTER_RELEASED = 0x0601;
constexpr uint16_t PRESS_TO_FOCUS = 0x0701;
constexpr uint16_t HOLD_FOCUS = 0x0801;
constexpr uint16_t TAKE_PICTURE = 0x0901;

// constexpr uint16_t SHUTTER_RELEASED = 0x0106;
// constexpr uint16_t PRESS_TO_FOCUS = 0x0107;
// constexpr uint16_t HOLD_FOCUS = 0x0108;
// constexpr uint16_t TAKE_PICTURE = 0x0109;

enum Mode
{
    MANUAL_FOCUS,
    AUTO_FOCUS
};

enum CameraCommand
{
    ShutterReleased,
    PressToFocus,
    HoldFocus,
    TakePicture
};

class SonyCamera
{
public:
    SonyCamera();

    void startScan();
    void loop();
    bool isConnected();
    bool send(CameraCommand command);

    void trigger();
    void focus();

    bool doConnect = false;
    bool connected = false;
    uint32_t scanTime = 0; /** 0 = scan forever */

    NimBLEAdvertisedDevice *advDevice;
    // ClientCallbacks *clientCB;

private:
    NimBLERemoteCharacteristic *remoteCommandChr = nullptr;

    uint8_t shutterStatus;   // A0
    uint8_t focusStatus;     // 3F
    uint8_t recordingStatus; // D5

    uint32_t last_message;

    Mode mode = AUTO_FOCUS;
    bool focusHeld = false;

    bool connectToServer();
    void handleCameraNotification(uint8_t *data, uint16_t len);
};
