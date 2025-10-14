/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include "xinput_device.h"
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "common/tusb_common.h"
/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug. */
/* This is a composite device with 2x CDC and 1x XInput. */

#define USB_VID 0x045E
#define USB_PID 0x123 // 2x CDC + 1x Vendor/XInput
#define USB_BCD 0x0200

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,

    // This is a composite device. The class code is set to MISC with IAD protocol
    // as required by the USB specification for devices with multiple interfaces.
    .bDeviceClass = 0x0,
    .bDeviceSubClass = 0x0,
    .bDeviceProtocol = 0x0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_CDC_0 = 0,  // Interface 0 (CDC0 Comm)
  ITF_NUM_CDC_0_DATA, // Interface 1 (CDC0 Data)
  ITF_NUM_CDC_1,      // Interface 2 (CDC1 Comm)
  ITF_NUM_CDC_1_DATA,
  ITF_NUM_XINPUT,
  // ITF_NUM_CDC_2,
  // ITF_NUM_CDC_2_DATA,
  ITF_NUM_TOTAL,
};

// total length of configuration descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + TUD_XINPUT_DESC_LEN)

// define endpoint numbers
#define EPNUM_CDC_0_NOTIF 0x81 // notification endpoint for CDC 0
#define EPNUM_CDC_0_OUT 0x08   // out endpoint for CDC 0
#define EPNUM_CDC_0_IN 0x88    // in endpoint for CDC 0

#define EPNUM_CDC_1_NOTIF 0x84 // notification endpoint for CDC 1
#define EPNUM_CDC_1_OUT 0x05   // out endpoint for CDC 1
#define EPNUM_CDC_1_IN 0x85    // in endpoint for CDC 1

#define EPNUM_XINPUT_OUT 0x02 // out endpoint for XINPUT
#define EPNUM_XINPUT_IN 0x82  // in endpoint for XINPUT

// full speed configuration
uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 4, EPNUM_CDC_1_NOTIF, 8, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 64),
    TUD_XINPUT_DESCRIPTOR(ITF_NUM_XINPUT, 5, EPNUM_XINPUT_OUT, EPNUM_XINPUT_IN, 32),

};

TU_VERIFY_STATIC(sizeof(desc_fs_configuration) == CONFIG_TOTAL_LEN, "Configuration descriptor size is incorrect.");

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations
  return desc_fs_configuration;
}

#define MS_OS_1_0_VENDOR_CODE 0x90

static const uint8_t ms_os_ext_compat_id[40] = {
    // Header (16 bytes)
    0x28, 0x00, 0x00, 0x00,
    0x00, 0x01,
    0x04, 0x00,
    0x01,                // 1 function section
    0, 0, 0, 0, 0, 0, 0, // reserved[7]

    // Function Section (24 bytes)
    0x04, // bFirstInterface = 4
    0x00, // reserved
    'X', 'U', 'S', 'B', '2', '2', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
//--------------------------------------------------------------------+
// E. TINYUSB VENDOR CALLBACK (for MS OS 2.0)
//--------------------------------------------------------------------+

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
  if (stage != CONTROL_STAGE_SETUP)
    return true;
  if (request->bmRequestType == 0xC0 && // Device-to-host, Vendor
      request->bRequest == MS_OS_1_0_VENDOR_CODE &&
      request->wIndex == 0x0004)
  {
    // Return 40-byte descriptor
    return tud_control_xfer(rhport, request, (void *)ms_os_ext_compat_id, sizeof(ms_os_ext_compat_id));
  }
  return false;
}
//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum
{
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

// array of pointer to string descriptors
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "Tak",                      // 1: Manufacturer
    "Tak's Device",             // 2: Product
    NULL,                       // 3: Serials will use unique ID if possible
    "Tak's CDC Interface",      // 4: CDC Interface
    "Tak's Roller",             // 5: HID Interface
    "MSFT100",
};

static uint16_t _desc_str[32 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  size_t chr_count;

  switch (index)
  {
  case STRID_LANGID:
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
    break;

  case STRID_SERIAL:
    chr_count = board_usb_get_serial(_desc_str + 1, 32);
    break;

  case 0xEE:
    // UTF-16LE encode "MSFT100" + vendor code byte
    static uint16_t msft100_desc[9];
    // length = 2*(7 chars) + 2(header) + 2(vendor code) = 18 bytes
    msft100_desc[0] = (TUSB_DESC_STRING << 8) | 18;
    // 'M','S','F','T','1','0','0'
    const char *s = "MSFT100";
    for (uint8_t i = 0; i < 7; i++)
    {
      msft100_desc[1 + i] = s[i];
    }
    // vendor code in low byte of the next UTF-16LE char
    msft100_desc[8] = MS_OS_1_0_VENDOR_CODE;
    return msft100_desc;
    break;
  default:
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char *str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
    if (chr_count > max_count)
      chr_count = max_count;

    // Convert ASCII string into UTF-16
    for (size_t i = 0; i < chr_count; i++)
    {
      _desc_str[1 + i] = str[i];
    }
    break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}