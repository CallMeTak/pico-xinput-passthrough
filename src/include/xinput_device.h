#ifdef __cplusplus
extern "C"
{
#endif
#include "pico/stdlib.h"
#include "tusb_config.h"

#define TUD_XINPUT_CLASS 0xFF
#define TUD_XINPUT_SUBCLASS 0x5D
#define TUD_XINPUT_PROTOCOL 0x01
#define TUD_XINPUT_DESC_LEN 0x09 + 0x10 + 7 + 7

#ifndef CFG_TUD_XINPUT_EPIN_BUFSIZE
#define CFG_TUD_XINPUT_EPIN_BUFSIZE 32
#endif

#ifndef CFG_TUD_XINPUT_EPOUT_BUFSIZE
#define CFG_TUD_XINPUT_EPOUT_BUFSIZE 32
#endif

    typedef struct __attribute__((packed, aligned(1)))
    {
        uint8_t bReportID;
        uint8_t bSize;
        uint16_t bmButtons;
        uint8_t bLeftTrigger;
        uint8_t bRightTrigger;
        int16_t wThumbLeftX;
        int16_t wThumbLeftY;
        int16_t wThumbRightX;
        int16_t wThumbRightY;
        uint8_t reserved[6];
    } xinput_report_t;

    typedef struct
    {
        uint16_t bmButtons;
        uint8_t bLeftTrigger;
        uint8_t bRightTrigger;
        int16_t wThumbLeftX;
        int16_t wThumbLeftY;
        int16_t wThumbRightX;
        int16_t wThumbRightY;
    } xinput_state_t;

    typedef enum
    {
        BUTTON_DPAD_UP = (1 << 0),
        BUTTON_DPAD_DOWN = (1 << 1),
        BUTTON_DPAD_LEFT = (1 << 2),
        BUTTON_DPAD_RIGHT = (1 << 3),
        BUTTON_START = (1 << 4),
        BUTTON_BACK = (1 << 5),
        BUTTON_LEFT_THUMB = (1 << 6),
        BUTTON_RIGHT_THUMB = (1 << 7),
        BUTTON_LEFT_SHOULDER = (1 << 8),
        BUTTON_RIGHT_SHOULDER = (1 << 9),
        BUTTON_XE = (1 << 10),
        BUTTON_BINDING = (1 << 11),
        BUTTON_A = (1 << 12),
        BUTTON_B = (1 << 13),
        BUTTON_X = (1 << 14),
        BUTTON_Y = (1 << 15)
    } XboxButton;

    /// @brief Send a user-supplied XInput report to the host.
    /// @param report
    /// @return true if the report was successfully queued for sending, false otherwise.
    bool tud_xinput_report(xinput_report_t *report);

    /// @brief Equivalent to tud_xinput_report, but sends the internal gamepad state
    /// @return
    bool tud_xinput_update();

    /// @brief Updates the current state of the gamepad. Does not send a report.
    /// @param state
    /// @return
    /// This is a helper method useful when using xinput_host library (for example in a passthrough application)
    void tud_xinput_update_state(xinput_state_t *state);

    void tud_xinput_press_button(XboxButton button);
    void tud_xinput_release_button(XboxButton button);

    void tud_xinput_joystick_lx(int16_t value);

    void tud_xinput_joystick_ly(int16_t value);

    void tud_xinput_joystick_rx(int16_t value);

    void tud_xinput_joystick_ry(int16_t value);

#define TUD_XINPUT_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize) \
        /* Interface */  \
        0x09, TUSB_DESC_INTERFACE, _itfnum, 0, 2, TUD_XINPUT_CLASS, TUD_XINPUT_SUBCLASS, TUD_XINPUT_PROTOCOL, _stridx,  \
        /* Unknown HID */  \
        0x10, 0x21, 0x10, 0x01, 0x01, 0x24, _epin, 0x14, 0x03, 0x00, 0x03, 0x13, _epout, 0x00, 0x03, 0x00, \
        /* Endpoint In */  \
        7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 1,   \
        /* Endpoint Out */ \
        7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 8

#ifdef __cplusplus
}
#endif