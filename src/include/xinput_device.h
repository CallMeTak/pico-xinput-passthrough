#include "pico/stdlib.h"
#include "tusb_config.h"
void receive_xinput_report(void);
bool send_xinput_report(void *report, uint8_t report_size);

#define TUD_XINPUT_CLASS 0xFF
#define TUD_XINPUT_SUBCLASS 0x5D
#define TUD_XINPUT_PROTOCOL 0x01
#define TUD_XINPUT_DESC_LEN 0x09 + 0x10 + 7 + 7
#define CFG_TUD_XINPUT_EPIN_BUFSIZE 32
#define CFG_TUD_XINPUT_EPOUT_BUFSIZE 32
typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t bReportID;
    uint8_t bSize;
    uint16_t bmButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    uint16_t wThumbLeftX;
    uint16_t wThumbLeftY;
    uint16_t wThumbRightX;
    uint16_t wThumbRightY;
    uint8_t reserved[6];
} xinput_report_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint16_t bmButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    uint16_t wThumbLeftX;
    uint16_t wThumbLeftY;
    uint16_t wThumbRightX;
    uint16_t wThumbRightY;
} xinput_state_t;

typedef enum {
    BUTTON_DPAD_UP          = (1 << 0),
    BUTTON_DPAD_DOWN        = (1 << 1),
    BUTTON_DPAD_LEFT        = (1 << 2),
    BUTTON_DPAD_RIGHT       = (1 << 3),
    BUTTON_START            = (1 << 4),
    BUTTON_BACK             = (1 << 5),
    BUTTON_LEFT_THUMB       = (1 << 6),
    BUTTON_RIGHT_THUMB      = (1 << 7),
    BUTTON_LEFT_SHOULDER    = (1 << 8),
    BUTTON_RIGHT_SHOULDER   = (1 << 9),
    BUTTON_XE               = (1 << 10),
    BUTTON_BINDING          = (1 << 11),
    BUTTON_A                = (1 << 12),
    BUTTON_B                = (1 << 13),
    BUTTON_X                = (1 << 14),
    BUTTON_Y                = (1 << 15)
} Button;

void tud_xinput_press_button(Button button);
void tud_xinput_release_button(Button button);

void tud_xinput_joystick_lx(uint16_t value);

void tud_xinput_joystick_ly(uint16_t value);

void tud_xinput_joystick_rx(uint16_t value);

void tud_xinput_joystick_ry(uint16_t value);

void tud_xinput_update();
bool tud_xinput_send_report(xinput_report_t *report);


/*static const uint8_t xinput_configuration_descriptor[] =
{
	0x09,        // bLength
	0x02,        // bDescriptorType (Configuration)
	0x30, 0x00,  // wTotalLength 48
	0x01,        // bNumInterfaces 1
	0x01,        // bConfigurationValue
	0x00,        // iConfiguration (String Index)
	0x80,        // bmAttributes
	0xFA,        // bMaxPower 500mA

	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x00,        // bInterfaceNumber 0
	0x00,        // bAlternateSetting
	0x02,        // bNumEndpoints 2
	0xFF,        // bInterfaceClass
	0x5D,        // bInterfaceSubClass
	0x01,        // bInterfaceProtocol
	0x00,        // iInterface (String Index)

	0x10,        // bLength
	0x21,        // bDescriptorType (HID)
	0x10, 0x01,  // bcdHID 1.10
	0x01,        // bCountryCode
	0x24,        // bNumDescriptors
	0x81,        // bDescriptorType[0] (Unknown 0x81)
	0x14, 0x03,  // wDescriptorLength[0] 788
	0x00,        // bDescriptorType[1] (Unknown 0x00)
	0x03, 0x13,  // wDescriptorLength[1] 4867
	0x01,        // bDescriptorType[2] (Unknown 0x02)
	0x00, 0x03,  // wDescriptorLength[2] 768
	0x00,        // bDescriptorType[3] (Unknown 0x00)

	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x81,        // bEndpointAddress (IN/D2H)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x01,        // bInterval 1 (unit depends on device speed)

	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x01,        // bEndpointAddress (OUT/H2D)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x08,        // bInterval 8 (unit depends on device speed)
};
*/
#define TUD_XINPUT_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize) \
    /* Interface */\
    0x09, TUSB_DESC_INTERFACE, _itfnum, 0, 2, TUD_XINPUT_CLASS, TUD_XINPUT_SUBCLASS, TUD_XINPUT_PROTOCOL, _stridx,\
    /* Unknown HID */\
    0x10, 0x21, 0x10, 0x01, 0x01, 0x24, 0x81, 0x14, 0x03, 0x00, 0x03, 0x13, 0x02, 0x00, 0x03, 0x00,\
    /* Endpoint In */\
    7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 1,\
    /* Endpoint Out */\
    7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(_epsize), 8
