#include "xinput_device.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include "tusb_config.h"
#ifdef CFG_TUD_XINPUT
typedef struct
{
    xinput_state_t gamepad_state;
    xinput_report_t gamepad_report;
    uint8_t connected;
    uint8_t new_pad_data;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t epin_buf[CFG_TUD_XINPUT_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUD_XINPUT_EPOUT_BUFSIZE];

    xfer_result_t last_xfer_result;
    uint32_t last_xferred_bytes;
} xinputd_interface_t;

static xinputd_interface_t xinputd_itf;

// Application methods
void tud_xinput_press_button(Button button)
{
    xinputd_itf.gamepad_state.bmButtons |= button;
}

void tud_xinput_release_button(Button button)
{
    xinputd_itf.gamepad_state.bmButtons &= ~button;
}
void tud_xinput_joystick_lx(uint16_t value)
{
    xinputd_itf.gamepad_state.wThumbLeftX = value;
}
void tud_xinput_joystick_ly(uint16_t value)
{
    xinputd_itf.gamepad_state.wThumbLeftY = value;
}
void tud_xinput_joystick_rx(uint16_t value)
{
    xinputd_itf.gamepad_state.wThumbRightX = value;
}
void tud_xinput_joystick_ry(uint16_t value)
{
    xinputd_itf.gamepad_state.wThumbRightY = value;
}

// Driver callbacks
static uint16_t tud_xinput_open_cb(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
    // Verify that this is the XInput interface
    TU_VERIFY(itf_descriptor->bInterfaceClass == TUD_XINPUT_CLASS &&
              itf_descriptor->bInterfaceSubClass == TUD_XINPUT_SUBCLASS &&
              itf_descriptor->bInterfaceProtocol == TUD_XINPUT_PROTOCOL, 0);

    uint16_t driver_length = sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16;

    TU_VERIFY(max_length >= driver_length, 0);

    uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
    uint8_t found_endpoints = 0;
    while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length))
    {
        tusb_desc_endpoint_t const *endpoint_descriptor = (tusb_desc_endpoint_t const *)current_descriptor;
        // If endpoint found, open it
        if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor))
        {
            TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

            if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
                xinputd_itf.ep_in = endpoint_descriptor->bEndpointAddress;
            else
                xinputd_itf.ep_out = endpoint_descriptor->bEndpointAddress;

            ++found_endpoints;
        }

        current_descriptor = tu_desc_next(current_descriptor);
    }
    return driver_length;
}

bool tud_xinput_send_report(xinput_report_t *report)
{
    TU_VERIFY(usbd_edpt_claim(0, xinputd_itf.ep_in));
    memcpy(xinputd_itf.epin_buf, report, 20);
    // Return if tud not ready or endpoint busy
    if (!tud_ready() || usbd_edpt_busy(0, xinputd_itf.ep_in))
    {
        usbd_edpt_release(0, xinputd_itf.ep_in);
        return false;
    }

    // The transfer is asynchronous, the endpoint will be released in the xfer_cb
    return usbd_edpt_xfer(0, xinputd_itf.ep_in, xinputd_itf.epin_buf, 20);
}
void tud_xinput_update()
{
    tud_xinput_send_report(&xinputd_itf.gamepad_report);
}
bool tud_xinput_send_state(xinput_state_t *state){
    // Copy state into the local gamepad_report buffer
    TU_VERIFY_STATIC(sizeof(xinput_state_t) == sizeof(xinput_report_t) - 8, "REPORT STATE WRONG SIZE");
    memcpy(&xinputd_itf.gamepad_report + 2, state, sizeof(xinput_state_t));
    return true;
    //return tud_xinput_send_report(&xinputd_itf.gamepad_report);
}
void tud_xinput_init_cb(void)
{
    // Reset the interface state
    memset(&xinputd_itf, 0, sizeof(xinputd_itf));

    // Initialize the gamepad report with a compound literal
    xinputd_itf.gamepad_report = (xinput_report_t){
        .bReportID = 0x00,
        .bSize = 0x14, // 20 bytes for a standard XInput report
        // The rest of the fields will be zero-initialized
    };
}
bool tud_xinput_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    (void)rhport;
    (void)stage;
    (void)request;
    return true;
}
bool tud_xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    (void)result;
    (void)xferred_bytes;
    return true;
    // IN transfer completed, release the endpoint
    if (ep_addr == xinputd_itf.ep_in)
    {
        usbd_edpt_release(rhport, xinputd_itf.ep_in);
    }
    return true;
}
void tud_xinput_reset_cb(uint8_t rhport)
{
    (void)rhport;
    tud_xinput_init_cb();
}
usbd_class_driver_t const usbd_xinput_driver =
    {
        .name = "XINPUT_DEVICE",
        .init = tud_xinput_init_cb,
        .open = tud_xinput_open_cb,
        .control_xfer_cb = tud_xinput_control_xfer_cb,
        .xfer_cb = tud_xinput_xfer_cb,
        .reset = tud_xinput_reset_cb,
        .sof = NULL,
};

//usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
//{
//    *driver_count = 1;
//    return &usbd_xinput_driver;
//}

const uint8_t tud_xinput_report_desc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //   Logical Maximum (65535)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x10,        //   Usage Maximum (0x10)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x01,        //   Logical Minimum (1)
    0x25, 0x08,        //   Logical Maximum (8)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //   Logical Maximum (65535)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x04,        //   Report ID (4)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x09, 0x21,        //   Usage (0x21)
    0x09, 0x22,        //   Usage (0x22)
    0x09, 0x23,        //   Usage (0x23)
    0x09, 0x24,        //   Usage (0x24)
    0x09, 0x25,        //   Usage (0x25)
    0x09, 0x26,        //   Usage (0x26)
    0x09, 0x27,        //   Usage (0x27)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x08,        //   Report Count (8)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

uint8_t const * tud_hid_report_descriptor_cb(uint8_t itf)
{
  (void) itf;
  // This must return a pointer to your XInput report descriptor array.
  // The name of the array might be different, but it's the one
  // whose size is 114 bytes.
  return tud_xinput_report_desc;
}
// This is a community-standard HID report descriptor for an XInput-compatible device.
// It describes all the axes, buttons, and triggers in a format that the xusb22.sys
// driver can understand. The total length is 114 bytes.

#endif